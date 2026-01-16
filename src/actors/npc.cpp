#include "str_npc.h"
#include "str_npc_derived.h"
#include "str_npc_type.h"
#include "str_constants.h"
#include "str_collision.h"

#include "bn_math.h"
#include "bn_optional.h"
#include "bn_keypad.h"
#include "bn_sprite_text_generator.h"
#include "bn_sound_items.h"
#include "bn_sprite_builder.h"
#include "bn_fixed_point.h"
#include "bn_camera_ptr.h"
#include "bn_span.h"

#include "bn_sprite_items_merchant.h"
#include "common_variable_8x8_sprite_font.h"

namespace str {

    // =========================================================================
    // NPC Implementation
    // =========================================================================

    NPC::NPC(bn::fixed_point pos, bn::camera_ptr &camera, NPC_TYPE type, bn::sprite_text_generator &text_generator)
        : Entity(pos), _type(type), _camera(camera), _text_generator(text_generator) { _text_generator.set_bg_priority(0); }

    void NPC::update() {
        if (_action) _action->update();
        if (_is_talking) {
            if (_dialog_state == DIALOG_STATE::SHOWING_OPTIONS) { handle_option_navigation(); render_dialog_options(); return; }
            bool line_done = _currentChar >= _lines.at(_currentLine).size() * 2;
            if (line_done) {
                if (bn::keypad::up_pressed() || bn::keypad::a_pressed()) {
                    if (_currentLine == _lines.size() - 1) {
                        if (_has_dialog_options) { _dialog_state = _dialog_state == DIALOG_STATE::GREETING ? DIALOG_STATE::SHOWING_OPTIONS : _dialog_state;
                            if (_dialog_state == DIALOG_STATE::SHOWING_OPTIONS) { _selected_option = 0; _currentLine = 0; _currentChar = 0; _currentChars = ""; return; }
                        }
                        end_conversation(); return;
                    }
                    bn::sound_items::hello.play(); _currentLine++; _currentChar = 0; _currentChars = "";
                } else if (bn::keypad::start_pressed()) end_conversation();
            } else {
                if (bn::keypad::start_pressed()) end_conversation();
                else if (bn::keypad::a_pressed() || bn::keypad::up_pressed()) { _currentChar = _lines.at(_currentLine).size() * 2; _currentChars = _lines.at(_currentLine); }
                else {
                    static int hold_counter = 0;
                    if ((bn::keypad::a_held() || bn::keypad::up_held() ? ++hold_counter >= 2 : (hold_counter = 0, true)) && ++_currentChar >= 0) {
                        _currentChars = _lines.at(_currentLine).substr(0, _currentChar / 2 + 1);
                    }
                }
            }
            _text_generator.set_left_alignment(); _text_sprites.clear(); _text_generator.generate(-90, _text_y_limit, _currentChars, _text_sprites);
        } else if (_is_near_player && !_finished) {
            _text_generator.set_center_alignment(); _text_sprites.clear(); _text_generator.generate(0, _text_y_limit, "press 'A' to interact", _text_sprites);
        } else _text_sprites.clear();
    }

    bool NPC::finished_talking() { return _has_spoken_once; }
    bool NPC::is_in_interaction_zone(bn::fixed_point p) { if (!_finished && !_hidden) { if (bn::abs(pos().x() - p.x()) < str::MERCHANT_INTERACTION_ZONE_WIDTH && bn::abs(pos().y() - p.y()) < str::MERCHANT_INTERACTION_ZONE_HEIGHT) return _is_near_player = 1; _is_near_player = 0; } return 0; }
    bool NPC::check_trigger(bn::fixed_point p) { return is_in_interaction_zone(p); }
    void NPC::talk() { if (!_is_talking) { _is_talking = 1; _dialog_state = DIALOG_STATE::GREETING; _currentLine = _currentChar = 0; _currentChars = ""; _has_spoken_once = 1; bn::sound_items::hello.play(); } }
    bool NPC::is_talking() { return _is_talking; }
    void NPC::set_is_hidden(bool h) { _hidden = h; if (_sprite) _sprite->set_visible(!h); }
    bool NPC::hidden() { return _hidden; }
    void NPC::end_conversation() { _is_talking = 0; _currentChars = ""; _currentChar = _currentLine = 0; _dialog_state = DIALOG_STATE::GREETING; _has_spoken_once = 1; _text_sprites.clear(); }

    void NPC::render_dialog_options() {
        _text_sprites.clear(); _text_generator.set_left_alignment(); bn::fixed y = _text_y_limit - 20;
        for (int i = 0; i < _dialog_options.size(); ++i) { bn::string<64> t = (i == _selected_option ? "> " : "  "); t.append(_dialog_options[i].option_text); _text_generator.generate(-90, y, t, _text_sprites); y += _text_y_inc; }
    }

    void NPC::handle_option_navigation() {
        if (bn::keypad::down_pressed()) { bn::sound_items::hello.play(); _selected_option = (_selected_option + 1) % _dialog_options.size(); }
        else if (bn::keypad::up_pressed()) { bn::sound_items::hello.play(); _selected_option = (_selected_option - 1 + _dialog_options.size()) % _dialog_options.size(); }
        else if (bn::keypad::a_pressed()) select_dialog_option();
        else if (bn::keypad::start_pressed()) end_conversation();
    }

    void NPC::select_dialog_option() {
        if (_selected_option < _dialog_options.size()) { bn::sound_items::hello.play(); _lines = _dialog_options[_selected_option].response_lines; _dialog_state = _dialog_options[_selected_option].ends_conversation ? DIALOG_STATE::ENDING : DIALOG_STATE::SHOWING_RESPONSE; _currentLine = _currentChar = 0; _currentChars = ""; _last_char_count = -1; }
    }

    // =========================================================================
    // MerchantNPC Implementation
    // =========================================================================

    bn::string_view MerchantNPC::_dialogue_lines[3] = {
        "Hello there, traveler!",
        "I'm a wandering merchant.",
        "What can I help you with?"
    };
    
    bn::string_view MerchantNPC::_past_response_lines[4] = {
        "Ah, my past... well,",
        "I've traveled far and wide,",
        "trading goods across the lands.",
        "Every journey has a story!"
    };
    
    bn::string_view MerchantNPC::_directions_response_lines[3] = {
        "Looking for somewhere specific?",
        "Head north for the forest,",
        "or south to reach the desert."
    };
    
    bn::string_view MerchantNPC::_goodbye_response_lines[2] = {
        "Safe travels, friend!",
        "Come back anytime!"
    };

    MerchantNPC::MerchantNPC(bn::fixed_point pos, bn::camera_ptr &camera, bn::sprite_text_generator &text_generator)
        : NPC(pos, camera, NPC_TYPE::MERCHANT, text_generator) { initialize_sprite(); initialize_dialogue(); initialize_dialog_options(); }
    void MerchantNPC::initialize_sprite() { bn::sprite_builder b(bn::sprite_items::merchant); b.set_position(pos()); b.set_bg_priority(1); b.set_z_order(100); _sprite = b.build(); if (_sprite) set_camera(_camera); }
    void MerchantNPC::initialize_dialogue() { _lines = bn::span(_dialogue_lines); }
    void MerchantNPC::initialize_dialog_options() { _has_dialog_options = 1; _dialog_options.push_back(DialogOption("Ask about his past", bn::span(_past_response_lines), 0)); _dialog_options.push_back(DialogOption("Ask for directions", bn::span(_directions_response_lines), 0)); _dialog_options.push_back(DialogOption("Goodbye", bn::span(_goodbye_response_lines), 1)); }

} // namespace str
