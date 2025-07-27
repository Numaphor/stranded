#ifndef FE_HITBOX_DEBUG_H
#define FE_HITBOX_DEBUG_H

#include "bn_sprite_ptr.h"
#include "bn_camera_ptr.h"
#include "bn_optional.h"
#include "bn_vector.h"
#include "bn_regular_bg_ptr.h"
#include "bn_color.h"
#include "bn_colors.h"
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

        /**
         * @brief Structure to hold marker position adjustments
         */
        struct MarkerAdjustment
        {
            bn::fixed top_left_x_change;
            bn::fixed top_left_y_change;
            bn::fixed bottom_right_x_change;
            bn::fixed bottom_right_y_change;
        };

        /**
         * @brief Calculate marker adjustments based on hitbox changes (collision-based measurement)
         * @param hitbox_width_change Change in hitbox width (positive = wider collision area)
         * @param hitbox_height_change Change in hitbox height (positive = taller collision area)
         * @return MarkerAdjustment structure with calculated position changes for functional collision boundaries
         *
         * Note: This function uses collision-based measurement where hitbox represents the actual
         * functional collision distance between entities, not visual sprite boundaries.
         */
        [[nodiscard]] static MarkerAdjustment calculate_marker_adjustment(bn::fixed hitbox_width_change, bn::fixed hitbox_height_change);

        /**
         * @brief Apply marker adjustments to existing marker positions
         * @param top_left_x Reference to top-left marker X position
         * @param top_left_y Reference to top-left marker Y position
         * @param bottom_right_x Reference to bottom-right marker X position
         * @param bottom_right_y Reference to bottom-right marker Y position
         * @param adjustment The adjustment values to apply
         */
        static void apply_marker_adjustment(bn::fixed &top_left_x, bn::fixed &top_left_y,
                                            bn::fixed &bottom_right_x, bn::fixed &bottom_right_y,
                                            const MarkerAdjustment &adjustment);

        /**
         * @brief Centralized marker positioning configuration
         *
         * This struct contains all offset values for different entity types,
         * replacing scattered magic numbers with documented, maintainable constants.
         */
        struct MarkerOffsetConfig
        {
            bn::fixed top_left_x;
            bn::fixed top_left_y;
            bn::fixed bottom_right_x;
            bn::fixed bottom_right_y;

            // Constructor for easy initialization
            MarkerOffsetConfig(bn::fixed tl_x, bn::fixed tl_y, bn::fixed br_x, bn::fixed br_y)
                : top_left_x(tl_x), top_left_y(tl_y), bottom_right_x(br_x), bottom_right_y(br_y) {}
        };

        // Predefined configurations for different entity types
        static const MarkerOffsetConfig STANDARD_ENTITY_CONFIG;
        static const MarkerOffsetConfig PLAYER_CONFIG;
        static const MarkerOffsetConfig MERCHANT_ACTION_RADIUS_CONFIG;
        static const MarkerOffsetConfig MERCHANT_HITBOX_CONFIG;

    private:
        struct HitboxMarkers
        {
            bn::optional<bn::sprite_ptr> top_left;
            bn::optional<bn::sprite_ptr> bottom_right;
            // Additional markers for merchant hitbox visualization (distinct from action radius)
            bn::optional<bn::sprite_ptr> hitbox_top_left;
            bn::optional<bn::sprite_ptr> hitbox_bottom_right;

            void clear()
            {
                top_left.reset();
                bottom_right.reset();
                hitbox_top_left.reset();
                hitbox_bottom_right.reset();
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
                if (hitbox_top_left.has_value())
                {
                    hitbox_top_left->set_visible(visible);
                }
                if (hitbox_bottom_right.has_value())
                {
                    hitbox_bottom_right->set_visible(visible);
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
         * @brief Create or update merchant action radius markers with specialized positioning
         * @param hitbox The merchant hitbox to visualize (adjusted to show action radius)
         * @param markers The marker sprites to update
         */
        void _update_merchant_action_radius_markers(const Hitbox &hitbox, HitboxMarkers &markers);

        /**
         * @brief Create or update merchant hitbox markers with specialized positioning and red tinting
         * @param hitbox The merchant hitbox to visualize (actual collision boundaries)
         * @param markers The marker sprites to update
         *
         * Note: These markers use fixed positioning that is decoupled from MERCHANT_ACTION_RADIUS_CONFIG
         * to ensure the hitbox visualization remains at the original collision boundaries regardless
         * of any changes to the action radius configuration.
         */
        void _update_merchant_hitbox_markers(const Hitbox &hitbox, HitboxMarkers &markers);

        /**
         * @brief Unified marker update function using configuration-based positioning
         * @param hitbox The hitbox to visualize
         * @param markers The marker sprites to update
         * @param config The offset configuration to use
         * @param use_hitbox_markers If true, use hitbox_top_left/hitbox_bottom_right instead of top_left/bottom_right
         * @param enable_blending If true, enable blending for visual distinction
         */
        void _update_markers_with_config(const Hitbox &hitbox, HitboxMarkers &markers,
                                         const MarkerOffsetConfig &config,
                                         bool use_hitbox_markers = false,
                                         bool enable_blending = false);

        /**
         * @brief Create a new marker sprite
         * @param position Position for the marker
         * @param rotated True to rotate the marker 180 degrees
         * @return The created sprite
         */
        bn::sprite_ptr _create_marker(bn::fixed_point position, bool rotated = false);

        /**
         * @brief Calculate top-left marker position for a hitbox
         * @param hitbox The hitbox to position marker for
         * @param x_offset Additional X offset for visual adjustment
         * @param y_offset Additional Y offset for visual adjustment
         * @return Position for the top-left marker
         */
        [[nodiscard]] bn::fixed_point _calculate_top_left_marker_pos(const Hitbox &hitbox, bn::fixed x_offset = 0, bn::fixed y_offset = 0) const;

        /**
         * @brief Calculate bottom-right marker position for a hitbox
         * @param hitbox The hitbox to position marker for
         * @param x_offset Additional X offset for visual adjustment (negative moves left)
         * @param y_offset Additional Y offset for visual adjustment (negative moves up)
         * @return Position for the bottom-right marker
         */
        [[nodiscard]] bn::fixed_point _calculate_bottom_right_marker_pos(const Hitbox &hitbox, bn::fixed x_offset = 0, bn::fixed y_offset = 0) const;

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
