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

    void NPC::update()
    {
        if (_action.has_value())
        {
            _action.value().update();
        }

        if (_is_talking)
        {
            if (_currentChar == _lines.at(_currentLine).size() * 2)
            {
                if (bn::keypad::up_pressed() || bn::keypad::a_pressed() || bn::keypad::a_held() || bn::keypad::up_held())
                {
                    if (_currentLine == _lines.size() - 1)
                    {
                        _is_talking = false;
                        _currentChars = "";
                        _currentChar = 0;
                        _currentLine = 0;
                        _has_spoken_once = true;
                    }
                    else
                    {
                        bn::sound_items::hello.play();
                        _currentLine += 1;
                        _currentChar = 0;
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
                int char_count = (_currentChar / 2) + 1;
                if (char_count != _last_char_count) {
                    _currentChars = _lines.at(_currentLine).substr(0, char_count);
                    _last_char_count = char_count;
                }
                if (bn::keypad::a_held() || bn::keypad::up_held())
                {
                    _currentChar += 2;
                }
                else
                {
                    ++_currentChar;
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
            if (bn::abs(pos().x() - player_pos.x()) < 50)
            {
                if (bn::abs(pos().y() - player_pos.y()) < 50)
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
        _is_talking = true;
        bn::sound_items::hello.play();
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

}
