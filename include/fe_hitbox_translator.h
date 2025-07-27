#ifndef FE_HITBOX_TRANSLATOR_H
#define FE_HITBOX_TRANSLATOR_H

#include "bn_fixed.h"
#include "bn_fixed_point.h"
#include "bn_optional.h"
#include "fe_hitbox.h"

namespace fe
{
    namespace hitbox_translator
    {
        /**
         * @brief Marker positioning strategies based on analyzed successful patterns
         *
         * Each strategy represents a different approach to marker positioning that
         * has been proven to work well with specific entity types.
         */
        enum class MarkerStrategy
        {
            STANDARD,           // Direct hitbox corners (basic entities)
            ANIMATION_ADJUSTED, // For sprites with animation padding (like PLAYER)
            DUAL_AREA,          // For entities needing both interaction and collision areas (like MERCHANT)
            TILE_BASED          // For tile-boundary systems (like ZONE)
        };

        /**
         * @brief Animation padding offsets for sprites larger than their hitboxes
         *
         * These offsets help position markers correctly when sprites include
         * animation frames that make them larger than their functional collision area.
         */
        struct PaddingOffsets
        {
            bn::fixed x_offset = 0;
            bn::fixed y_offset = 0;
        };

        /**
         * @brief Complete marker positioning result
         *
         * Contains all marker positions needed for an entity. Some fields are optional
         * based on the marker strategy used.
         */
        struct MarkerPositions
        {
            bn::fixed_point top_left;
            bn::fixed_point bottom_right;

            // For dual-area entities (like MERCHANT)
            bn::optional<bn::fixed_point> interaction_top_left;
            bn::optional<bn::fixed_point> interaction_bottom_right;
        };

        /**
         * @brief Entity characteristics analysis result
         *
         * Helps determine the best marker strategy for an entity based on
         * its sprite and hitbox properties.
         */
        struct EntityCharacteristics
        {
            bool has_animation_frames = false;
            bool sprite_larger_than_hitbox = false;
            bool needs_interaction_area = false;
            bool is_tile_based = false;
            bn::fixed sprite_to_hitbox_ratio = 1;
        };

        /**
         * @brief Determine the best marker strategy for an entity
         *
         * @param has_animations True if entity has animation frames
         * @param has_interaction_area True if entity needs both interaction and collision zones
         * @param is_tile_based True if entity uses tile-based boundaries
         * @return Recommended marker strategy
         */
        [[nodiscard]] MarkerStrategy determine_marker_strategy(bool has_animations,
                                                               bool has_interaction_area,
                                                               bool is_tile_based);

        /**
         * @brief Calculate animation padding offsets based on sprite vs hitbox size
         *
         * Uses the PLAYER pattern: when sprites are larger due to animation frames,
         * calculate appropriate offsets to position markers on the functional area.
         *
         * @param sprite_width Width of the sprite (including animation padding)
         * @param sprite_height Height of the sprite (including animation padding)
         * @param hitbox_width Width of the functional hitbox
         * @param hitbox_height Height of the functional hitbox
         * @return Calculated padding offsets
         */
        [[nodiscard]] PaddingOffsets calculate_animation_padding(bn::fixed sprite_width,
                                                                 bn::fixed sprite_height,
                                                                 bn::fixed hitbox_width,
                                                                 bn::fixed hitbox_height);

        /**
         * @brief Universal marker position calculator
         *
         * Calculates marker positions based on the specified strategy and the
         * successful patterns from PLAYER, MERCHANT, and ZONE entities.
         *
         * @param hitbox The entity's hitbox
         * @param strategy Marker positioning strategy to use
         * @param padding Animation padding offsets (for ANIMATION_ADJUSTED strategy)
         * @return Complete marker positioning data
         */
        [[nodiscard]] MarkerPositions calculate_marker_positions(const Hitbox &hitbox,
                                                                 MarkerStrategy strategy,
                                                                 const PaddingOffsets &padding = {});

        /**
         * @brief Analyze entity characteristics to recommend strategy
         *
         * @param sprite_width Width of the entity's sprite
         * @param sprite_height Height of the entity's sprite
         * @param hitbox_width Width of the entity's hitbox
         * @param hitbox_height Height of the entity's hitbox
         * @param needs_interaction True if entity needs interaction area (like NPCs)
         * @param is_tile_based True if entity uses tile-based boundaries
         * @return Analysis of entity characteristics
         */
        [[nodiscard]] EntityCharacteristics analyze_entity(bn::fixed sprite_width,
                                                           bn::fixed sprite_height,
                                                           bn::fixed hitbox_width,
                                                           bn::fixed hitbox_height,
                                                           bool needs_interaction = false,
                                                           bool is_tile_based = false);

        /**
         * @brief Helper function for MERCHANT-style dual area positioning
         *
         * @param hitbox The merchant's hitbox
         * @return Marker positions for the interaction (action radius) area
         */
        [[nodiscard]] MarkerPositions calculate_merchant_action_radius_markers(const Hitbox &hitbox);

        /**
         * @brief Helper function for MERCHANT-style hitbox positioning
         *
         * @param hitbox The merchant's hitbox
         * @return Marker positions for the actual collision hitbox area
         */
        [[nodiscard]] MarkerPositions calculate_merchant_collision_markers(const Hitbox &hitbox);

        /**
         * @brief Helper function for ZONE-style tile-based positioning
         *
         * @param zone_min_x Minimum X coordinate of zone
         * @param zone_max_x Maximum X coordinate of zone
         * @param zone_min_y Minimum Y coordinate of zone
         * @param zone_max_y Maximum Y coordinate of zone
         * @param inset Inset distance from boundaries (default: 4 pixels)
         * @return Marker positions for the zone boundaries
         */
        [[nodiscard]] MarkerPositions calculate_zone_markers(bn::fixed zone_min_x,
                                                             bn::fixed zone_max_x,
                                                             bn::fixed zone_min_y,
                                                             bn::fixed zone_max_y,
                                                             bn::fixed inset = 4);

        /**
         * @brief Quick setup function for standard entities
         *
         * @param hitbox The entity's hitbox
         * @param sprite_width Width of sprite (0 if unknown)
         * @param sprite_height Height of sprite (0 if unknown)
         * @param needs_interaction True if entity needs interaction area
         * @return Complete marker positioning using best strategy
         */
        [[nodiscard]] MarkerPositions quick_setup_markers(const Hitbox &hitbox,
                                                          bn::fixed sprite_width = 0,
                                                          bn::fixed sprite_height = 0,
                                                          bool needs_interaction = false);
    }
}

#endif
