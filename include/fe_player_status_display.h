#ifndef FE_PLAYER_STATUS_DISPLAY_H
#define FE_PLAYER_STATUS_DISPLAY_H

#include "bn_sprite_ptr.h"
#include "bn_optional.h"
#include "bn_vector.h"
#include "bn_sprite_text_generator.h"

namespace fe
{
    class Player; // Forward declaration
    enum class PLAYER_STATUS
    {
        IDLE,
        MOVING,
        COMBAT,
        LISTENING,
        NEAR_MERCHANT,
        HIT,
        DEAD
    };

    class PlayerStatusDisplay
    {
    private:
        bn::vector<bn::sprite_ptr, 32> _text_sprites;
        bn::sprite_text_generator &_text_generator;
        PLAYER_STATUS _current_status = PLAYER_STATUS::IDLE;
        bool _is_visible = true;
        bool _status_valid = false;  // Track if status has been set

        static constexpr bn::fixed STATUS_X = 76; // X position for status display
        static constexpr bn::fixed STATUS_Y = 70; // Lower position

    public:
        PlayerStatusDisplay(bn::sprite_text_generator &text_generator);

        void update_status(const Player &player, bool near_merchant = false);
        void set_visible(bool is_visible);

    private:
        void _update_display();
        PLAYER_STATUS _determine_status(const Player &player, bool near_merchant);
        const char *_status_to_string(PLAYER_STATUS status);
    };
}

#endif
