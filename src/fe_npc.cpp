#include "fe_npc.h"
#include "fe_npc_type.h"

#include "bn_optional.h"
#include "bn_math.h"
#include "bn_log.h"
#include "bn_display.h"
#include "bn_keypad.h"
#include "bn_sprite_text_generator.h"

#include "bn_sound_items.h"

namespace fe
{

    NPC::NPC(bn::fixed_point pos, bn::camera_ptr &camera, NPC_TYPE type, bn::sprite_text_generator &text_generator)
        : Entity(pos), _type(type), _camera(camera), _text_generator(text_generator)
    {
        _text_generator.set_bg_priority(0);

        // NPCs don't use hitboxes for collision - all collision and interaction are handled
        // by the Level class tile-based system
    }

    void NPC::update()
    {
        if (_action.has_value())
        {
            _action.value().update();
        }

        if (_is_talking)
        {
            // Handle dialog option state
            if (_dialog_state == DIALOG_STATE::SHOWING_OPTIONS)
            {
                handle_option_navigation();
                render_dialog_options();
                return;
            }
            
            // Only process input if we're not waiting for the last line to finish
            if (_currentChar >= _lines.at(_currentLine).size() * 2)
            {
                if (bn::keypad::up_pressed() || bn::keypad::a_pressed())
                {
                    if (_currentLine == _lines.size() - 1)
                    {
                        // Check if we should show dialog options after greeting
                        if (_dialog_state == DIALOG_STATE::GREETING && _has_dialog_options)
                        {
                            _dialog_state = DIALOG_STATE::SHOWING_OPTIONS;
                            _selected_option = 0;
                            return;
                        }
                        // After showing response, return to options
                        else if (_dialog_state == DIALOG_STATE::SHOWING_RESPONSE && _has_dialog_options)
                        {
                            _dialog_state = DIALOG_STATE::SHOWING_OPTIONS;
                            _selected_option = 0;
                            _currentLine = 0;
                            _currentChar = 0;
                            _currentChars = "";
                            return;
                        }
                        // End conversation after last line
                        end_conversation();
                        return;
                    }
                    else
                    {
                        // Move to next line
                        bn::sound_items::hello.play();
                        _currentLine += 1;
                        _currentChar = 0;
                        _currentChars = "";
                    }
                }
                else if (bn::keypad::start_pressed())
                {
                    _is_talking = false;
                    _currentChars = "";
                    _currentChar = 0;
                    _currentLine = 0;
                    _dialog_state = DIALOG_STATE::GREETING;
                    _has_spoken_once = true;
                }
            }
            else
            {
                if (bn::keypad::start_pressed())
                {
                    _is_talking = false;
                    _currentChars = "";
                    _currentChar = 0;
                    _currentLine = 0;
                    _has_spoken_once = true;
                }
                else if ((bn::keypad::a_pressed() || bn::keypad::up_pressed()))
                {
                    // If text is still being displayed, skip to end of current line
                    if (_currentChar < _lines.at(_currentLine).size() * 2)
                    {
                        _currentChar = _lines.at(_currentLine).size() * 2;
                        _currentChars = _lines.at(_currentLine); // Show full line immediately
                        _last_char_count = _currentChars.size();
                    }
                }

                // Only auto-advance text if we're not already at the end
                if (_currentChar < _lines.at(_currentLine).size() * 2)
                {
                    int char_count = (_currentChar / 2) + 1;
                    if (char_count != _last_char_count)
                    {
                        _currentChars = _lines.at(_currentLine).substr(0, char_count);
                        _last_char_count = char_count;
                    }

                    // Always advance text, but faster when A/UP is held
                    static int hold_counter = 0;
                    bool should_advance = false;

                    if (bn::keypad::a_held() || bn::keypad::up_held())
                    {
                        // Faster text advancement when holding A/UP
                        if (++hold_counter >= 2)
                        { // Adjust this number for desired speed
                            should_advance = true;
                            hold_counter = 0;
                        }
                    }
                    else
                    {
                        // Normal speed when not holding
                        hold_counter = 0;
                        should_advance = true;
                    }

                    if (should_advance)
                    {
                        ++_currentChar;

                        // Check if we've reached the end of a line
                        if (_currentChar >= _lines.at(_currentLine).size() * 2)
                        {
                            // If this is the last line, wait for player input
                            if (_currentLine == _lines.size() - 1)
                            {
                                _currentChars = _lines.at(_currentLine); // Make sure full line is shown
                                _last_char_count = _currentChars.size();
                                // Reset character counter to prevent auto-advancing
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
        else if (_is_near_player && !_finished)
        {
            _text_generator.set_center_alignment();
            _text_sprites.clear();
            _text_generator.generate(0, _text_y_limit, "press 'A' to interact", _text_sprites);
        }
        else
        {
            _text_sprites.clear();
        }
    }

    bool NPC::finished_talking()
    {
        return _has_spoken_once;
    }

    bool NPC::is_in_interaction_zone(bn::fixed_point player_pos)
    {
        // LEGACY METHOD: This method is kept for compatibility but is no longer used for merchants
        // Merchant interaction is now handled by Level::is_in_merchant_interaction_zone() tile-based system
        // This method is still used by other NPC types that don't use the Level tile system
        if (!_finished && !_hidden)
        {
            if (bn::abs(pos().x() - player_pos.x()) < fe::MERCHANT_INTERACTION_ZONE_WIDTH)
            {
                if (bn::abs(pos().y() - player_pos.y()) < fe::MERCHANT_INTERACTION_ZONE_HEIGHT)
                {
                    _is_near_player = true;
                    return true;
                }
            }
            _is_near_player = false;
        }
        return false;
    }

    bool NPC::check_trigger(bn::fixed_point player_pos)
    {
        return is_in_interaction_zone(player_pos);
    }

    void NPC::talk()
    {
        // Only start talking if we're not already in the middle of a conversation
        if (!_is_talking)
        {
            _is_talking = true;
            _dialog_state = DIALOG_STATE::GREETING;
            _currentLine = 0;
            _currentChar = 0;
            _currentChars = "";
            _has_spoken_once = true;
            bn::sound_items::hello.play();
        }
    }

    bool NPC::is_talking()
    {
        return _is_talking;
    }

    void NPC::set_is_hidden(bool is_hidden)
    {
        _hidden = is_hidden;
        if (_sprite.has_value())
        {
            _sprite.value().set_visible(!is_hidden);
        }
    }

    bool NPC::hidden()
    {
        return _hidden;
    }

    void NPC::end_conversation()
    {
        _is_talking = false;
        _currentChars = "";
        _currentChar = 0;
        _currentLine = 0;
        _dialog_state = DIALOG_STATE::GREETING;
        _has_spoken_once = true;
        _text_sprites.clear();
        // Don't reset _is_near_player here - let the scene world handle listening state properly
    }

    void NPC::render_dialog_options()
    {
        _text_sprites.clear();
        _text_generator.set_left_alignment();
        
        constexpr int max_option_text_length = 64; // Max characters for option text including cursor
        bn::fixed y_pos = _text_y_limit - 20;
        
        for (int i = 0; i < _dialog_options.size(); ++i)
        {
            bn::string<max_option_text_length> option_text;
            if (i == _selected_option)
            {
                option_text = "> ";
            }
            else
            {
                option_text = "  ";
            }
            option_text.append(_dialog_options[i].option_text);
            
            _text_generator.generate(-90, y_pos, option_text, _text_sprites);
            y_pos += _text_y_inc;
        }
    }

    void NPC::handle_option_navigation()
    {
        if (bn::keypad::down_pressed())
        {
            bn::sound_items::hello.play();
            _selected_option = (_selected_option + 1) % _dialog_options.size();
        }
        else if (bn::keypad::up_pressed())
        {
            bn::sound_items::hello.play();
            _selected_option = (_selected_option - 1 + _dialog_options.size()) % _dialog_options.size();
        }
        else if (bn::keypad::a_pressed())
        {
            select_dialog_option();
        }
        else if (bn::keypad::start_pressed())
        {
            end_conversation();
        }
    }

    void NPC::select_dialog_option()
    {
        if (_selected_option < _dialog_options.size())
        {
            bn::sound_items::hello.play();
            
            // Load the response lines for the selected option
            _lines = _dialog_options[_selected_option].response_lines;
            
            // Check if this option should end the conversation
            if (_dialog_options[_selected_option].ends_conversation)
            {
                _dialog_state = DIALOG_STATE::GREETING; // Will end after showing response
            }
            else
            {
                _dialog_state = DIALOG_STATE::SHOWING_RESPONSE; // Will return to options
            }
            
            _currentLine = 0;
            _currentChar = 0;
            _currentChars = "";
            _last_char_count = -1;
        }
    }

}
