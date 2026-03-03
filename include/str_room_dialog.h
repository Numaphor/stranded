#ifndef STR_ROOM_DIALOG_H
#define STR_ROOM_DIALOG_H

#include <bn_sprite_text_generator.h>
#include <bn_sprite_ptr.h>
#include <bn_vector.h>
#include <bn_string_view.h>
#include <bn_keypad.h>
#include <bn_fixed.h>

namespace str
{

// Lightweight dialog system for the room viewer, inspired by the legacy NPC dialog.
// Renders typewriter text + selectable options using a provided text generator.
// No Entity / camera dependency -- works entirely in screen-space.
class RoomDialog
{
public:
    struct DialogOption
    {
        bn::string_view option_text;
        bn::span<const bn::string_view> response_lines;
        bool ends_conversation;
    };

    RoomDialog(bn::sprite_text_generator& tg) :
        _tg(tg),
        _active(false),
        _state(STATE_IDLE),
        _current_line(0),
        _current_char(0),
        _last_rendered_chars(-1),
        _hold_counter(0),
        _selected_option(0),
        _scroll_offset(0),
        _hud_sprites(nullptr)
    {
    }

    void set_greeting(bn::span<const bn::string_view> lines)
    {
        _greeting_lines = lines;
    }

    // Set pointer to HUD text sprites so we can clear them during dialog
    void set_hud_sprites(bn::ivector<bn::sprite_ptr>& hud)
    {
        _hud_sprites = &hud;
    }

    void set_options(bn::span<const DialogOption> options)
    {
        _options.clear();
        for(const DialogOption& opt : options)
        {
            _options.push_back(opt);
        }
    }

    void talk()
    {
        _active = true;
        _state = STATE_GREETING;
        _current_lines = _greeting_lines;
        _current_line = 0;
        _current_char = 0;
        _last_rendered_chars = -1;
        _hold_counter = 0;
        _selected_option = 0;
        _scroll_offset = 0;
        _text_sprites.clear();
        // Free HUD tiles so we have VRAM for dialog text
        if(_hud_sprites)
        {
            _hud_sprites->clear();
        }
    }

    // Returns true while dialog is active (caller should block player movement)
    [[nodiscard]] bool is_active() const
    {
        return _active;
    }

    // Call once per frame while active
    void update()
    {
        if(!_active)
        {
            return;
        }

        if(_state == STATE_SHOWING_OPTIONS)
        {
            _handle_option_navigation();
            _render_options();
            return;
        }

        // Typewriter text state (GREETING or SHOWING_RESPONSE)
        if(_current_line < _current_lines.size())
        {
            const bn::string_view& line = _current_lines[_current_line];
            int line_len = line.size();

            // Advance typewriter
            if(_current_char < line_len * 2)
            {
                bool advance_held = bn::keypad::a_held() || bn::keypad::up_held();
                if(advance_held)
                {
                    ++_hold_counter;
                }
                else
                {
                    _hold_counter = 0;
                }

                // Speed up when held
                int speed = (_hold_counter >= 2) ? 4 : 1;
                _current_char += speed;
                if(_current_char > line_len * 2)
                {
                    _current_char = line_len * 2;
                }

                // Only re-render when visible character count changes
                int chars_shown = _current_char / 2 + 1;
                if(chars_shown > line_len) chars_shown = line_len;

                if(chars_shown != _last_rendered_chars)
                {
                    _last_rendered_chars = chars_shown;
                    _text_sprites.clear();
                    bn::string_view sub(line.data(), chars_shown);

                    _tg.set_left_alignment();
                    _tg.set_bg_priority(0);
                    if(!_tg.generate_optional(TEXT_X, TEXT_Y, sub, _text_sprites))
                    {
                        // VRAM exhausted — stop
                    }
                }
            }
            else
            {
                // Line fully shown -- wait for A press to advance
                if(bn::keypad::a_pressed() || bn::keypad::up_pressed())
                {
                    _current_line++;
                    _current_char = 0;
                    _last_rendered_chars = -1;
                    _hold_counter = 0;

                    if(_current_line >= _current_lines.size())
                    {
                        // Finished all lines
                        if(_state == STATE_GREETING && !_options.empty())
                        {
                            _state = STATE_SHOWING_OPTIONS;
                            _selected_option = 0;
                            _scroll_offset = 0;
                            _text_sprites.clear();
                        }
                        else
                        {
                            _end();
                        }
                    }
                }
            }
        }

        // Start button ends dialog immediately
        if(bn::keypad::start_pressed())
        {
            _end();
        }
    }

private:
    static constexpr int TEXT_X = -90;
    static constexpr int TEXT_Y = 40;
    static constexpr int OPTION_Y_START = 30;
    static constexpr int OPTION_Y_INC = 12;
    static constexpr int VISIBLE_OPTIONS = 3;

    enum State
    {
        STATE_IDLE,
        STATE_GREETING,
        STATE_SHOWING_OPTIONS,
        STATE_SHOWING_RESPONSE
    };

    void _end()
    {
        _active = false;
        _state = STATE_IDLE;
        _current_line = 0;
        _current_char = 0;
        _last_rendered_chars = -1;
        _hold_counter = 0;
        _text_sprites.clear();
    }

    void _render_options()
    {
        _text_sprites.clear();
        _tg.set_left_alignment();
        _tg.set_bg_priority(0);

        int visible = _options.size() < VISIBLE_OPTIONS ?
                      _options.size() : VISIBLE_OPTIONS;

        for(int i = 0; i < visible; ++i)
        {
            int idx = _scroll_offset + i;
            if(idx >= _options.size()) break;

            int y = OPTION_Y_START + i * OPTION_Y_INC;
            if(idx == _selected_option)
            {
                // Selected prefix
                bn::string<64> sel_text("> ");
                sel_text.append(bn::string_view(_options[idx].option_text));
                if(!_tg.generate_optional(TEXT_X, y, sel_text, _text_sprites))
                {
                    break;
                }
            }
            else
            {
                bn::string<64> opt_text("  ");
                opt_text.append(bn::string_view(_options[idx].option_text));
                if(!_tg.generate_optional(TEXT_X, y, opt_text, _text_sprites))
                {
                    break;
                }
            }
        }
    }

    void _handle_option_navigation()
    {
        if(bn::keypad::down_pressed())
        {
            if(_selected_option < _options.size() - 1)
            {
                ++_selected_option;
                if(_selected_option >= _scroll_offset + VISIBLE_OPTIONS)
                {
                    ++_scroll_offset;
                }
            }
        }
        else if(bn::keypad::up_pressed())
        {
            if(_selected_option > 0)
            {
                --_selected_option;
                if(_selected_option < _scroll_offset)
                {
                    --_scroll_offset;
                }
            }
        }
        else if(bn::keypad::a_pressed())
        {
            _select_option(_selected_option);
        }
        else if(bn::keypad::start_pressed())
        {
            _end();
        }
    }

    void _select_option(int idx)
    {
        const DialogOption& opt = _options[idx];

        if(opt.ends_conversation)
        {
            _end();
            return;
        }

        if(opt.response_lines.size() > 0)
        {
            _state = STATE_SHOWING_RESPONSE;
            _current_lines = opt.response_lines;
            _current_line = 0;
            _current_char = 0;
            _last_rendered_chars = -1;
            _hold_counter = 0;
            _text_sprites.clear();
        }
        else
        {
            _end();
        }
    }

    bn::sprite_text_generator& _tg;
    bn::vector<bn::sprite_ptr, 32> _text_sprites;
    bn::vector<DialogOption, 8> _options;
    bn::span<const bn::string_view> _greeting_lines;
    bn::span<const bn::string_view> _current_lines;
    bool _active;
    State _state;
    int _current_line;
    int _current_char;
    int _last_rendered_chars;
    int _hold_counter;
    int _selected_option;
    int _scroll_offset;
    bn::ivector<bn::sprite_ptr>* _hud_sprites;
};

} // namespace str

#endif // STR_ROOM_DIALOG_H
