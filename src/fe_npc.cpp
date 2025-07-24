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
    }

    void NPC::update_hitbox()
    {
        // Center the 32x32 NPC hitbox on the NPC position
        // This matches the visual sprite positioning
        _hitbox.set_x(_pos.x() - 16); // Center horizontally (32/2)
        _hitbox.set_y(_pos.y() - 16); // Center vertically (32/2)
    }

    void NPC::update()
    {
        if (_action.has_value())
        {
            _action.value().update();
        }

        if (_is_talking)
        {
            // Only process input if we're not waiting for the last line to finish
            if (_currentChar >= _lines.at(_currentLine).size() * 2)
            {
                if (bn::keypad::up_pressed() || bn::keypad::a_pressed())
                {
                    if (_currentLine == _lines.size() - 1)
                    {
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
            _text_generator.generate(-110, _text_y_limit, _currentChars, _text_sprites);
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

    bool NPC::check_trigger(bn::fixed_point player_pos)
    {
        if (!_finished && !_hidden)
        {
            if (bn::abs(pos().x() - player_pos.x()) < 25)
            {
                if (bn::abs(pos().y() - player_pos.y()) < 25)
                {
                    _is_near_player = true;
                    return true;
                }
            }
            _is_near_player = false;
        }
        return false;
    }

    void NPC::talk()
    {
        // Only start talking if we're not already in the middle of a conversation
        if (!_is_talking)
        {
            _is_talking = true;
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
        _has_spoken_once = true;
        _text_sprites.clear();
        // Don't reset _is_near_player here - let the scene world handle listening state properly
    }

}
