#include "fe_player_companion.h"
#include "bn_sprite_items_companion.h"
#include "bn_sprite_items_companion_load.h"
#include "bn_sprite_animate_actions.h"
#include "common_variable_8x8_sprite_font.h"

namespace fe
{
    PlayerCompanion::PlayerCompanion(bn::sprite_ptr sprite)
        : _sprite(bn::move(sprite)),
          _position(0, 0),
          _position_side(Position::RIGHT),
          _is_dead(false),
          _is_flying(false),
          _player_too_close(false),
          _follow_delay(0),
          _target_offset(24, 0),
          _independent_death(false),
          _death_position(0, 0),
          _can_be_revived(false),
          _is_reviving(false),
          _revival_in_progress(false),
          _revival_timer(0)
    {
        // Don't set z_order here - let the dynamic z_order system handle it
        // The player's update_z_order() method will manage companion z_order based on position
    }

    void PlayerCompanion::spawn(bn::fixed_point pos, bn::camera_ptr camera)
    {
        // Don't override position if companion died independently and is staying at death position
        if (!_independent_death)
        {
            _position = pos + bn::fixed_point(8, -8);
        }
        _target_offset = calculate_companion_offset();
        _sprite.set_camera(camera);
        update_animation();

        // FOR TESTING: Spawn companion dead
        die_independently();
    }

    void PlayerCompanion::update(bn::fixed_point player_pos, bool player_is_dead)
    {
        // Handle player death affecting companion
        if (player_is_dead != _is_dead && !_independent_death && !_is_reviving)
        {
            _is_dead = player_is_dead;
            update_animation();
        }

        // If companion is reviving, handle revival animation
        if (_is_reviving)
        {
            // Stay at death position during revival
            _sprite.set_position(_death_position);

            // Check if revival animation is complete
            if (_animation && _animation->done())
            {
                // Revival complete - companion is now alive
                _is_reviving = false;
                _is_dead = false;
                _independent_death = false;
                _position = _death_position; // Start from death position
                update_animation();          // Switch to normal alive animation
            }
        }
        // If companion died independently, it should stay dead until revived
        else if (_independent_death)
        {
            // Stay at death position
            _sprite.set_position(_death_position);

            // Show/hide revival text based on player proximity and revival state
            if (_can_be_revived && !_revival_in_progress)
            {
                // Calculate distance to see if player is close enough
                bn::fixed_point diff = player_pos - _death_position;
                bn::fixed distance_sq = diff.x() * diff.x() + diff.y() * diff.y();
                bool player_in_range = (distance_sq <= REVIVE_DISTANCE * REVIVE_DISTANCE);

                if (player_in_range && _text_sprites.empty())
                {
                    show_revival_text();
                }
                else if (!player_in_range && !_text_sprites.empty())
                {
                    hide_revival_text();
                }
            }
            else if (_text_sprites.size() > 0)
            {
                hide_revival_text();
            }
        }
        else if (!_is_dead)
        {
            update_position(player_pos);
        }

        // Update animation - for death/revival animation, only update while not done
        if (_animation && (!_is_dead || !_animation->done() || _is_reviving))
        {
            _animation->update();

            // If death animation just finished and this was an independent death, mark as revivable
            if (_is_dead && _independent_death && _animation->done() && !_can_be_revived && !_is_reviving)
            {
                _can_be_revived = true;
            }
        }
    }

    void PlayerCompanion::update_position(bn::fixed_point player_pos)
    {
        // Define proximity thresholds for idle behavior
        constexpr bn::fixed IDLE_DISTANCE = 12;   // Stop moving when player gets this close
        constexpr bn::fixed RESUME_DISTANCE = 20; // Resume following when player moves this far away

        // Calculate direct distance from companion to player
        bn::fixed_point companion_to_player = player_pos - _position;
        bn::fixed player_distance = bn::sqrt(companion_to_player.x() * companion_to_player.x() +
                                             companion_to_player.y() * companion_to_player.y());

        // Update proximity state with hysteresis to prevent oscillation
        if (!_player_too_close && player_distance < IDLE_DISTANCE)
        {
            _player_too_close = true;
        }
        else if (_player_too_close && player_distance > RESUME_DISTANCE)
        {
            _player_too_close = false;
        }

        // Only move if player is not too close
        if (!_player_too_close)
        {
            bn::fixed_point target_pos = player_pos + _target_offset;
            bn::fixed_point diff = target_pos - _position;
            bn::fixed distance = bn::sqrt(diff.x() * diff.x() + diff.y() * diff.y());

            if (distance > 1)
            {
                bn::fixed speed = (distance * 0.08 < 1.2) ? distance * 0.08 : 1.2;
                speed = (speed > 0.3) ? speed : 0.3;
                _position += (diff / distance) * speed;
            }
        }

        // Update position side more frequently for responsive sprite direction changes
        // Allow this even when not moving so companion can face the right direction
        if (player_distance > 8)
        {
            bn::fixed_point offset = _position - player_pos;
            Position new_side;

            // Check if companion is primarily above or below the player
            if (bn::abs(offset.y()) > bn::abs(offset.x()))
            {
                // Companion is primarily above or below player
                if (offset.y() < 0)
                {
                    // Companion is above player - should look down
                    // Use left/right animation based on horizontal offset
                    new_side = (offset.x() >= 0) ? Position::RIGHT : Position::LEFT;
                }
                else
                {
                    // Companion is below player
                    new_side = Position::BELOW;
                }
            }
            else
            {
                // Companion is primarily to the left or right of player
                new_side = (offset.x() > 0) ? Position::RIGHT : Position::LEFT;
            }

            set_position_side(new_side);
        }

        _sprite.set_position(_position);
    }

    bn::fixed_point PlayerCompanion::calculate_companion_offset() const
    {
        switch (_position_side)
        {
        case Position::RIGHT:
            return {16, 0};
        case Position::LEFT:
            return {-16, 0};
        case Position::BELOW:
            return {0, 12};
        default:
            return {16, 0};
        }
    }

    void PlayerCompanion::update_animation()
    {
        if (_is_reviving)
        {
            // Play death animation in reverse for revival
            _animation = bn::create_sprite_animate_action_once(
                _sprite, 8, bn::sprite_items::companion.tiles_item(),
                21, 20, 19, 18, 17, 16, 15, 14, 13, 12);
        }
        else if (_is_dead)
        {
            _animation = bn::create_sprite_animate_action_once(
                _sprite, 8, bn::sprite_items::companion.tiles_item(),
                12, 13, 14, 15, 16, 17, 18, 19, 20, 21);
        }
        else
        {
            int start_frame = static_cast<int>(_position_side) * 4;
            _animation = bn::create_sprite_animate_action_forever(
                _sprite, 12, bn::sprite_items::companion.tiles_item(),
                start_frame, start_frame + 1, start_frame + 2, start_frame + 3);
        }
    }

    void PlayerCompanion::set_flying(bool flying)
    {
        _is_flying = flying;
        update_animation();
    }

    void PlayerCompanion::set_camera(bn::camera_ptr camera)
    {
        _sprite.set_camera(camera);
    }

    void PlayerCompanion::set_position_side(Position side)
    {
        if (_position_side != side)
        {
            _position_side = side;
            _target_offset = calculate_companion_offset();
            update_animation();
        }
    }

    void PlayerCompanion::set_visible(bool visible)
    {
        _sprite.set_visible(visible);
    }

    void PlayerCompanion::set_z_order(int z_order)
    {
        _sprite.set_z_order(z_order);
    }

    void PlayerCompanion::start_death_animation()
    {
        _is_dead = true;
        update_animation();
    }

    void PlayerCompanion::die_independently()
    {
        if (!_is_dead) // Only die if not already dead
        {
            _is_dead = true;
            _independent_death = true;
            _death_position = _position; // Remember where we died
            _can_be_revived = false;     // Will be set to true when death animation completes

            // Cancel any revival in progress
            cancel_revival();

            update_animation();
        }
    }

    bool PlayerCompanion::try_revive(bn::fixed_point player_pos, bool a_pressed, bool a_held)
    {
        if (!_independent_death || !_can_be_revived)
        {
            return false;
        }

        // Calculate squared distance between player and dead companion
        bn::fixed_point diff = player_pos - _death_position;
        bn::fixed distance_sq = diff.x() * diff.x() + diff.y() * diff.y();

        bool player_in_range = (distance_sq <= REVIVE_DISTANCE * REVIVE_DISTANCE);

        if (!player_in_range)
        {
            // Player moved away, cancel any revival in progress
            if (_revival_in_progress)
            {
                cancel_revival();
            }
            return false;
        }

        // Player is in range
        if (!_revival_in_progress)
        {
            // Start revival process when A is first pressed
            if (a_pressed)
            {
                _revival_in_progress = true;
                _revival_timer = 0;

                // Create progress bar sprite above the dead companion
                _progress_bar_sprite = bn::sprite_items::companion_load.create_sprite(
                    _death_position.x(), _death_position.y(), 0); // Start with frame 0 (empty)

                if (_sprite.camera().has_value())
                {
                    _progress_bar_sprite->set_camera(_sprite.camera().value());
                }
                _progress_bar_sprite->set_z_order(_sprite.z_order() - 1); // In front of companion
            }
        }
        else
        {
            // Revival in progress
            if (a_held)
            {
                // Continue revival
                _revival_timer++;

                // Update progress bar frame (8 frames for 300 total ticks)
                int progress_frame = (_revival_timer * 8) / REVIVAL_DURATION;
                if (progress_frame > 7)
                    progress_frame = 7; // Clamp to max frame

                if (_progress_bar_sprite.has_value())
                {
                    _progress_bar_sprite->set_tiles(bn::sprite_items::companion_load.tiles_item(), progress_frame);
                    // Keep progress bar positioned above companion
                    _progress_bar_sprite->set_position(_death_position.x() + 12, _death_position.y());
                }

                // Check if revival is complete
                if (_revival_timer >= REVIVAL_DURATION)
                {
                    // Revival complete - start revival animation
                    _revival_in_progress = false;
                    _revival_timer = 0;
                    _is_reviving = true;
                    _can_be_revived = false;     // Prevent multiple revival attempts
                    _position = _death_position; // Start revival at death position

                    // Hide progress bar
                    _progress_bar_sprite.reset();

                    update_animation(); // Start reverse animation
                    return true;
                }
            }
            else
            {
                // Player released A button, cancel revival
                cancel_revival();
            }
        }

        return false;
    }

    void PlayerCompanion::cancel_revival()
    {
        _revival_in_progress = false;
        _revival_timer = 0;

        // Hide progress bar
        if (_progress_bar_sprite.has_value())
        {
            _progress_bar_sprite.reset();
        }

        // Hide revival text
        hide_revival_text();
    }

    void PlayerCompanion::show_revival_text()
    {
        if (!_text_sprites.empty())
            return; // Already showing

        bn::sprite_text_generator text_generator(common::variable_8x8_sprite_font);
        text_generator.set_center_alignment();

        // Create text sprites at position above the dead companion
        bn::fixed_point text_pos = _death_position + bn::fixed_point(0, -20); // 20 pixels above companion
        text_generator.set_bg_priority(0);
        text_generator.generate(text_pos, "Press A to revive", _text_sprites);

        // Set camera for text sprites
        for (bn::sprite_ptr &text_sprite : _text_sprites)
        {
            text_sprite.set_camera(_sprite.camera());
            text_sprite.set_z_order(-32767); // Ensure text is on top
        }
    }

    void PlayerCompanion::hide_revival_text()
    {
        _text_sprites.clear();
    }
}
