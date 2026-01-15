#include "fe_npc.h"
#include "fe_npc_type.h"
#include "bn_optional.h"
#include "bn_math.h"
#include "bn_log.h"
#include "bn_display.h"
#include "bn_keypad.h"
#include "bn_sprite_text_generator.h"
#include "bn_sound_items.h"
namespace fe {
    NPC::NPC(bn::fixed_point pos, bn::camera_ptr &camera, NPC_TYPE type, bn::sprite_text_generator &text_generator)
        : Entity(pos), _type(type), _camera(camera), _text_generator(text_generator) { _text_generator.set_bg_priority(0); }
    void NPC::update() {
        if (_action.has_value()) { _action.value().update(); }
        if (_is_talking) {
            if (_dialog_state == DIALOG_STATE::SHOWING_OPTIONS) {
                handle_option_navigation();
                render_dialog_options();
                return; }
            if (_currentChar >= _lines.at(_currentLine).size() * 2) {
                if (bn::keypad::up_pressed() || bn::keypad::a_pressed()) {
                    if (_currentLine == _lines.size() - 1) {
                        if (_dialog_state == DIALOG_STATE::GREETING && _has_dialog_options) {
                            _dialog_state = DIALOG_STATE::SHOWING_OPTIONS;
                            _selected_option = 0;
                            return; }
                        else if (_dialog_state == DIALOG_STATE::SHOWING_RESPONSE && _has_dialog_options) {
                            _dialog_state = DIALOG_STATE::SHOWING_OPTIONS;
                            _selected_option = 0;
                            _currentLine = 0;
                            _currentChar = 0;
                            _currentChars = "";
                            return; }
                        end_conversation();
                        return; }
                    else {
                        bn::sound_items::hello.play();
                        _currentLine += 1;
                        _currentChar = 0;
                        _currentChars = ""; }
                }
                else if (bn::keypad::start_pressed()) {
                    _is_talking = false;
                    _currentChars = "";
                    _currentChar = 0;
                    _currentLine = 0;
                    _dialog_state = DIALOG_STATE::GREETING;
                    _has_spoken_once = true; }
            }
            else {
                if (bn::keypad::start_pressed()) {
                    _is_talking = false;
                    _currentChars = "";
                    _currentChar = 0;
                    _currentLine = 0;
                    _has_spoken_once = true; }
                else if ((bn::keypad::a_pressed() || bn::keypad::up_pressed())) {
                    if (_currentChar < _lines.at(_currentLine).size() * 2) {
                        _currentChar = _lines.at(_currentLine).size() * 2;
                        _currentChars = _lines.at(_currentLine);
                        _last_char_count = _currentChars.size(); }
                }
                if (_currentChar < _lines.at(_currentLine).size() * 2) {
                    int char_count = (_currentChar / 2) + 1;
                    if (char_count != _last_char_count) {
                        _currentChars = _lines.at(_currentLine).substr(0, char_count);
                        _last_char_count = char_count; }
                    static int hold_counter = 0;
                    bool should_advance = false;
                    if (bn::keypad::a_held() || bn::keypad::up_held()) {
                        if (++hold_counter >= 2) {
                            should_advance = true;
                            hold_counter = 0; }
                    }
                    else {
                        hold_counter = 0;
                        should_advance = true; }
                    if (should_advance) {
                        ++_currentChar;
                        if (_currentChar >= _lines.at(_currentLine).size() * 2) {
                            if (_currentLine == _lines.size() - 1) {
                                _currentChars = _lines.at(_currentLine);
                                _last_char_count = _currentChars.size();
                                _currentChar = _lines.at(_currentLine).size() * 2;
                            }
                        }
                    }
                }
            }
            _text_generator.set_left_alignment();
            _text_sprites.clear();
            _text_generator.generate(-90, _text_y_limit, _currentChars, _text_sprites);
        }
        else if (_is_near_player && !_finished) {
            _text_generator.set_center_alignment();
            _text_sprites.clear();
            _text_generator.generate(0, _text_y_limit, "press 'A' to interact", _text_sprites);
        }
        else { _text_sprites.clear(); }
    }
    bool NPC::finished_talking() { return _has_spoken_once; }
    bool NPC::is_in_interaction_zone(bn::fixed_point player_pos) {
        if (!_finished && !_hidden) {
            if (bn::abs(pos().x() - player_pos.x()) < fe::MERCHANT_INTERACTION_ZONE_WIDTH) {
                if (bn::abs(pos().y() - player_pos.y()) < fe::MERCHANT_INTERACTION_ZONE_HEIGHT) {
                    _is_near_player = true;
                    return true; }
            }
            _is_near_player = false; }
        return false; }
    bool NPC::check_trigger(bn::fixed_point player_pos) { return is_in_interaction_zone(player_pos); }
    void NPC::talk() {
        if (!_is_talking) {
            _is_talking = true;
            _dialog_state = DIALOG_STATE::GREETING;
            _currentLine = 0;
            _currentChar = 0;
            _currentChars = "";
            _has_spoken_once = true;
            bn::sound_items::hello.play(); }
    }
    bool NPC::is_talking() { return _is_talking; }
    void NPC::set_is_hidden(bool is_hidden) {
        _hidden = is_hidden;
        if (_sprite.has_value()) { _sprite.value().set_visible(!is_hidden); }
    }
    bool NPC::hidden() { return _hidden; }
    void NPC::end_conversation() {
        _is_talking = false;
        _currentChars = "";
        _currentChar = 0;
        _currentLine = 0;
        _dialog_state = DIALOG_STATE::GREETING;
        _has_spoken_once = true;
        _text_sprites.clear(); }
    void NPC::render_dialog_options() {
        _text_sprites.clear();
        _text_generator.set_left_alignment();
        constexpr int max_option_text_length = 64;
        bn::fixed y_pos = _text_y_limit - 20;
        for (int i = 0; i < _dialog_options.size(); ++i) {
            bn::string<max_option_text_length> option_text;
            if (i == _selected_option) { option_text = "> "; }
            else { option_text = "  "; }
            option_text.append(_dialog_options[i].option_text);
            _text_generator.generate(-90, y_pos, option_text, _text_sprites);
            y_pos += _text_y_inc; }
    }
    void NPC::handle_option_navigation() {
        if (bn::keypad::down_pressed()) {
            bn::sound_items::hello.play();
            _selected_option = (_selected_option + 1) % _dialog_options.size(); }
        else if (bn::keypad::up_pressed()) {
            bn::sound_items::hello.play();
            _selected_option = (_selected_option - 1 + _dialog_options.size()) % _dialog_options.size();
        }
        else if (bn::keypad::a_pressed()) { select_dialog_option(); }
        else if (bn::keypad::start_pressed()) { end_conversation(); }
    }
    void NPC::select_dialog_option() {
        if (_selected_option < _dialog_options.size()) {
            bn::sound_items::hello.play();
            _lines = _dialog_options[_selected_option].response_lines;
            if (_dialog_options[_selected_option].ends_conversation) { _dialog_state = DIALOG_STATE::ENDING; }
            else { _dialog_state = DIALOG_STATE::SHOWING_RESPONSE; }
            _currentLine = 0;
            _currentChar = 0;
            _currentChars = "";
            _last_char_count = -1; }
    }
}
#include "fe_npc_derived.h"
#include "bn_sprite_animate_actions.h"
#include "bn_sprite_builder.h"
namespace fe {
    bn::string_view MerchantNPC::_dialogue_lines[3] = {
        "Hello there, traveler!",
        "I'm a wandering merchant.",
        "What can I help you with?"};
    bn::string_view MerchantNPC::_past_response_lines[4] = {
        "Ah, my past... well,",
        "I've traveled far and wide,",
        "trading goods across the lands.",
        "Every journey has a story!"};
    bn::string_view MerchantNPC::_directions_response_lines[3] = {
        "Looking for somewhere specific?",
        "Head north for the forest,",
        "or south to reach the desert."};
    bn::string_view MerchantNPC::_goodbye_response_lines[2] = {
        "Safe travels, friend!",
        "Come back anytime!"};
    MerchantNPC::MerchantNPC(bn::fixed_point pos, bn::camera_ptr &camera, bn::sprite_text_generator &text_generator)
        : NPC(pos, camera, NPC_TYPE::MERCHANT, text_generator) {
        initialize_sprite();
        initialize_dialogue();
        initialize_dialog_options(); }
    void MerchantNPC::initialize_sprite() {
        bn::sprite_builder builder(bn::sprite_items::merchant);
        builder.set_position(pos());
        builder.set_bg_priority(1);
        builder.set_z_order(100);
        _sprite = builder.build();
        if (_sprite.has_value()) { set_camera(_camera); }
    }
    void MerchantNPC::initialize_dialogue() { _lines = bn::span(_dialogue_lines); }
    void MerchantNPC::initialize_dialog_options() {
        _has_dialog_options = true;
        DialogOption past_option("Ask about his past", bn::span(_past_response_lines), false);
        _dialog_options.push_back(past_option);
        DialogOption directions_option("Ask for directions", bn::span(_directions_response_lines), false);
        _dialog_options.push_back(directions_option);
        DialogOption goodbye_option("Goodbye", bn::span(_goodbye_response_lines), true);
        _dialog_options.push_back(goodbye_option); }
}