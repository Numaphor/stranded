#ifndef FE_HITBOX_DEBUG_H
#define FE_HITBOX_DEBUG_H

#include "bn_sprite_ptr.h"
#include "bn_camera_ptr.h"
#include "bn_optional.h"
#include "bn_vector.h"
#include "bn_regular_bg_ptr.h"
#include "fe_hitbox.h"
#include "fe_npc_type.h"

namespace fe
{
    // Forward declarations
    class Player;
    class Enemy;
    class NPC;

    /**
     * @brief Debug visualization system for hitboxes
     *
     * This class manages the visualization of hitboxes for debugging purposes.
     * It uses the hitbox_marker sprite to show the corners of entity hitboxes.
     *
     * The top-left 4x4 portion of the hitbox_marker sprite is used as markers:
     * - One marker placed at the top-left corner of the hitbox
     * - One marker rotated 180 degrees and placed at the bottom-right corner
     */
    class HitboxDebug
    {
    public:
        HitboxDebug();

        /**
         * @brief Initialize the debug system with a camera
         * @param camera Camera to use for the debug sprites
         */
        void initialize(bn::camera_ptr camera);

        /**
         * @brief Update hitbox visualization for a player
         * @param player Player to visualize hitboxes for
         */
        void update_player_hitbox(const Player &player);

        /**
         * @brief Update hitbox visualization for an enemy
         * @param enemy Enemy to visualize hitboxes for
         */
        void update_enemy_hitbox(const Enemy &enemy);

        /**
         * @brief Update hitbox visualization for an NPC
         * @param npc NPC to visualize hitboxes for
         */
        void update_npc_hitbox(const NPC &npc);

        /**
         * @brief Update zone tiles visualization
         * @param level Level containing zone tile information
         * @param bg Background containing the tile map
         */
        void update_zone_tiles(const class Level &level, const bn::regular_bg_ptr &bg);

        /**
         * @brief Clear all hitbox visualizations
         */
        void clear_all();

        /**
         * @brief Enable or disable hitbox visualization
         * @param enabled True to show hitboxes, false to hide them
         */
        void set_enabled(bool enabled);

        /**
         * @brief Check if hitbox visualization is enabled
         * @return True if enabled, false otherwise
         */
        [[nodiscard]] bool is_enabled() const { return _enabled; }

    private:
        struct HitboxMarkers
        {
            bn::optional<bn::sprite_ptr> top_left;
            bn::optional<bn::sprite_ptr> bottom_right;

            void clear()
            {
                top_left.reset();
                bottom_right.reset();
            }

            void set_visible(bool visible)
            {
                if (top_left.has_value())
                {
                    top_left->set_visible(visible);
                }
                if (bottom_right.has_value())
                {
                    bottom_right->set_visible(visible);
                }
            }
        };

        /**
         * @brief Create or update hitbox markers for a given hitbox
         * @param hitbox The hitbox to visualize
         * @param markers The marker sprites to update
         */
        void _update_markers(const Hitbox &hitbox, HitboxMarkers &markers);

        /**
         * @brief Create or update player hitbox markers with adjusted positioning
         * @param hitbox The player hitbox to visualize
         * @param markers The marker sprites to update
         */
        void _update_player_markers(const Hitbox &hitbox, HitboxMarkers &markers);

        /**
         * @brief Create or update merchant hitbox markers with specialized positioning
         * @param hitbox The merchant hitbox to visualize
         * @param markers The marker sprites to update
         */
        void _update_merchant_markers(const Hitbox &hitbox, HitboxMarkers &markers);

        /**
         * @brief Create a new marker sprite
         * @param position Position for the marker
         * @param rotated True to rotate the marker 180 degrees
         * @return The created sprite
         */
        bn::sprite_ptr _create_marker(bn::fixed_point position, bool rotated = false);

        bn::optional<bn::camera_ptr> _camera;
        HitboxMarkers _player_markers;
        bn::vector<HitboxMarkers, 32> _enemy_markers; // Support up to 32 enemies
        bn::vector<HitboxMarkers, 32> _npc_markers;   // Support up to 32 NPCs
        HitboxMarkers _zone_markers;                  // Single marker pair for the entire zone area

        // Zone caching to avoid expensive calculations every frame
        bool _zone_bounds_cached;
        bn::fixed _zone_min_x, _zone_max_x, _zone_min_y, _zone_max_y;

        bool _enabled;
        bool _initialized;
    };
}

#endif // FE_HITBOX_DEBUG_H
