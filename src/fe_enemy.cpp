#include "fe_enemy.h"
#include "bn_fixed_point.h"
#include "bn_sprite_ptr.h"
#include "bn_camera_ptr.h"
#include "bn_regular_bg_ptr.h"
#include "bn_regular_bg_map_ptr.h"
#include "bn_optional.h"
#include "bn_span.h"
#include "bn_log.h"
#include "bn_size.h"
#include "bn_sprite_builder.h"

#include "bn_sprite_items_spearguard.h"

#include "fe_level.h"
#include "fe_enemy_type.h"
#include "fe_collision.h"

namespace fe
{
    // Animation frame ranges based on issue requirements
    struct AnimationFrameRange {
        int start;
        int end;
        int speed; // frames per animation frame
    };
    
    const AnimationFrameRange IDLE_FRAMES = {0, 5, 8};    // frames 0-5, 8 game frames per animation frame
    const AnimationFrameRange RUN_FRAMES = {6, 9, 6};     // frames 6-9, 6 game frames per animation frame  
    const AnimationFrameRange ATTACK_FRAMES = {10, 14, 4}; // frames 10-14, 4 game frames per animation frame
    const AnimationFrameRange DEAD_FRAMES = {15, 30, 8};   // frames 15-30, 8 game frames per animation frame

    [[nodiscard]] int _get_map_cell(bn::fixed x, bn::fixed y, bn::regular_bg_ptr & /*map*/, bn::span<const bn::regular_bg_map_cell> cells)
    {
        int map_x = ((x.integer() + 384) / 8);
        int map_y = ((y.integer() + 256) / 8);
        return cells.at(map_y * (768 / 8) + map_x);
    }

    [[nodiscard]] bool _contains_cell(int tile, bn::vector<int, 32> tiles)
    {
        for (int i = 0; i < tiles.size(); ++i)
        {
            if (tiles.at(i) == tile)
            {
                return true;
            }
        }
        return false;
    }

    [[nodiscard]] bool _check_collisions_map(bn::fixed_point pos, Hitbox hitbox, directions direction, bn::regular_bg_ptr &map, Level level, bn::span<const bn::regular_bg_map_cell> cells)
    {
        bn::fixed_point points[4];
        hitbox.get_collision_points(pos, direction, points);

        for (int i = 0; i < 4; ++i)
        {
            int tile = _get_map_cell(points[i].x(), points[i].y(), map, cells);
            if (_contains_cell(tile, level.floor_tiles()))
            {
                return true;
            }
        }
        return false;
    }

    Enemy::Enemy(int x, int y, bn::camera_ptr camera, bn::regular_bg_ptr map, ENEMY_TYPE type, int hp) : Entity(bn::fixed_point(x, y)),
                                                                                                         _camera(camera),
                                                                                                         _type(type),
                                                                                                         _dir(0),
                                                                                                         _hp(hp),
                                                                                                         _map(map),
                                                                                                         _map_cells(map.map().cells_ref().value())
    {
        // Only use spearguard sprite and advanced animations for spearguard enemies
        // For other enemy types, they would need their own sprite items to be implemented
        bn::sprite_builder builder(bn::sprite_items::spearguard);
        builder.set_position(pos());
        builder.set_bg_priority(1);

        _sprite = builder.build();
        if (!_sprite.has_value())
        {
            return;
        }

        set_camera(_camera);
        
        _hitbox = Hitbox(pos().x() - 4, pos().y() - 4, 8, 8);

        // Setup initial animation with multiple frames
        _action = bn::create_sprite_animate_action_forever(
            *_sprite,
            8,
            bn::sprite_items::spearguard.tiles_item(),
            0, 1, 2, 3 // Multiple frames for animation
        );
    }

    void Enemy::update_hitbox()
    {
        // Adjust hitbox position to account for center-based sprite positioning
        _hitbox.set_x(pos().x() - 4); // Adjust for smaller hitbox
        _hitbox.set_y(pos().y() - 4);
    }

    void Enemy::update(bn::fixed_point player_pos, const Level &level, bool player_listening)
    {
        // Handle death state first - if dead, only update animation and return (spearguard only)
        if (_dead && _type == ENEMY_TYPE::MUTANT)
        {
            if (_death_timer == 0)
            {
                _death_timer = 1; // Start death animation
                _state = EnemyState::DEAD;
            }
            _death_timer++;
            
            // Update animation and return - dead enemies don't move
            _update_animation();
            if (_sprite.has_value() && _action.has_value())
            {
                _action->update();
            }
            return;
        }

        // Handle attack state (spearguard only)
        if (_type == ENEMY_TYPE::MUTANT && _attacking && _attack_timer > 0)
        {
            _attack_timer--;
            if (_attack_timer <= 0)
            {
                _attacking = false;
                _state = EnemyState::IDLE; // Return to idle after attack
            }
            
            // During attack, enemy doesn't move but still updates animation
            _update_animation();
            if (_sprite.has_value() && _action.has_value())
            {
                _action->update();
            }
            return;
        }

        // Handle knockback first
        if (_knockback_timer > 0)
        {
            _knockback_timer--;

            // Apply knockback movement
            set_position(bn::fixed_point(pos().x() + _knockback_dx, pos().y() + _knockback_dy));

            // Apply friction to knockback
            _knockback_dx *= 0.9;
            _knockback_dy *= 0.9;

            // If knockback is done, clear the state
            if (_knockback_timer == 0)
            {
                _knockback_dx = 0;
                _knockback_dy = 0;
                _stunned = false;
            }

            // Update hitbox and return early - don't process normal AI during knockback
            update_hitbox();
            return;
        }

        // More natural movement: idle/walk/follow states, smooth velocity
        static bn::random random;
        _state_timer++;

        // Distance to player - cache expensive sqrt calculation
        bn::fixed dist_x = player_pos.x() - pos().x();
        bn::fixed dist_y = player_pos.y() - pos().y();
        bn::fixed dist_sq = dist_x * dist_x + dist_y * dist_y;
        const bn::fixed attack_dist_sq = 24 * 24;   // 3 tiles squared - attack range
        const bn::fixed follow_dist_sq = 48 * 48;   // 6 tiles squared
        const bn::fixed unfollow_dist_sq = 64 * 64; // 8 tiles squared

        // Attack logic - if very close to player and not already attacking (spearguard only)
        if (_type == ENEMY_TYPE::MUTANT && !player_listening && !_attacking && dist_sq <= attack_dist_sq)
        {
            _attacking = true;
            _attack_timer = 30; // Attack animation duration (30 frames = 0.5 seconds at 60fps)
            _state = EnemyState::ATTACK;
            _state_timer = 0;
        }
        // Aggro logic - enemies can't detect player when they're listening to NPCs
        else if (!player_listening && _state != EnemyState::FOLLOW && dist_sq <= follow_dist_sq)
        {
            _state = EnemyState::FOLLOW;
            _state_timer = 0;
        }
        else if (_state == EnemyState::FOLLOW && (dist_sq > unfollow_dist_sq || player_listening))
        {
            _state = EnemyState::IDLE;
            _state_timer = 0;
            _target_dx = 0;
            _target_dy = 0;
            // Random idle duration
            _state_duration = 20 + (random.get() % 40);
        }

        // State logic
        if (_state == EnemyState::FOLLOW)
        {
            // Move toward player, slow lerp - use cached distance calculation
            bn::fixed speed = 0.35;
            bn::fixed len = bn::sqrt(dist_sq);
            if (len > 0.1)
            {
                _target_dx = (dist_x / len) * speed;
                _target_dy = (dist_y / len) * speed;
            }
            else
            {
                _target_dx = 0;
                _target_dy = 0;
            }
        }
        else
        {
            // Wander/idle as before
            if (_state_timer >= _state_duration)
            {
                _state_timer = 0;
                if (_state == EnemyState::IDLE)
                {
                    _state = EnemyState::WALK;
                    // Pick random direction (angle for wandering)
                    int angle = random.get() % 360;
                    bn::fixed radians = angle * 3.14159 / 180;
                    _target_dx = 0.35 * bn::sin(radians);
                    _target_dy = 0.35 * bn::cos(radians);
                    // Random walk duration (30-120 frames)
                    _state_duration = 30 + (random.get() % 90);
                }
                else // WALK -> IDLE
                {
                    _state = EnemyState::IDLE;
                    _target_dx = 0;
                    _target_dy = 0;
                    // Random idle duration (20-60 frames)
                    _state_duration = 20 + (random.get() % 40);
                }
            }
        }

        // Smoothly interpolate _dx/_dy to _target_dx/_target_dy
        const bn::fixed lerp = 0.1;
        _dx += (_target_dx - _dx) * lerp;
        _dy += (_target_dy - _dy) * lerp;
        
        // Update movement component
        _movement.set_velocity(bn::fixed_point(_dx, _dy));
        _movement.update();

        // Update position
        bn::fixed new_x = pos().x() + _dx;
        bn::fixed new_y = pos().y() + _dy;
        bn::fixed_point new_pos(new_x, new_y);

        // --- Robust collision check with proper direction ---
        // Determine movement direction for collision check
        fe::directions check_direction = fe::directions::down; // Default
        if (bn::abs(_dx) > bn::abs(_dy))
        {
            // Moving horizontally
            check_direction = _dx > 0 ? fe::directions::right : fe::directions::left;
        }
        else
        {
            // Moving vertically
            check_direction = _dy > 0 ? fe::directions::down : fe::directions::up;
        }

        if (fe::Collision::check_hitbox_collision_with_level(_hitbox, new_pos, check_direction, level))
        {
            set_position(new_pos);
        }
        else
        {
            // Try to slide along walls by checking individual axis movement
            bool can_move_x = true;
            bool can_move_y = true;

            // Check X movement only
            if (_dx != 0)
            {
                bn::fixed_point x_pos(pos().x() + _dx, pos().y());
                fe::directions x_dir = _dx > 0 ? fe::directions::right : fe::directions::left;
                can_move_x = fe::Collision::check_hitbox_collision_with_level(_hitbox, x_pos, x_dir, level);
            }

            // Check Y movement only
            if (_dy != 0)
            {
                bn::fixed_point y_pos(pos().x(), pos().y() + _dy);
                fe::directions y_dir = _dy > 0 ? fe::directions::down : fe::directions::up;
                can_move_y = fe::Collision::check_hitbox_collision_with_level(_hitbox, y_pos, y_dir, level);
            }

            // Apply movement on valid axes
            bn::fixed_point current_pos = pos();
            if (can_move_x)
            {
                current_pos.set_x(current_pos.x() + _dx);
            }
            else
            {
                _dx = 0;
                _movement.set_velocity(bn::fixed_point(_dx, _dy));
            }

            if (can_move_y)
            {
                current_pos.set_y(current_pos.y() + _dy);
            }
            else
            {
                _dy = 0;
                _movement.set_velocity(bn::fixed_point(_dx, _dy));
            }

            set_position(current_pos);
        }

        // Handle invulnerability timer
        if (_invulnerable)
        {
            if (--_inv_timer <= 0)
            {
                _invulnerable = false;
                _inv_timer = 0;
                if (_sprite.has_value())
                {
                    _sprite->set_visible(true);
                }
            }
        }

        // Update sprite if available
        if (_sprite.has_value())
        {
            _sprite->set_position(pos());
            _sprite->set_horizontal_flip(_dx < 0);
        }
        
        // Update animation system
        _update_animation();
        if (_action.has_value())
        {
            _action->update();
        }
    }

    void Enemy::set_pos(bn::fixed_point new_pos)
    {
        set_position(new_pos);
    }

    bool Enemy::_take_damage(int damage)
    {
        if (_invulnerable || _dead)
        {
            return false;
        }

        _hp -= damage;
        _invulnerable = true;
        _inv_timer = 30; // Invincibility frames
        _stunned = true;

        if (_hp <= 0)
        {
            _dead = true;
            if (_type == ENEMY_TYPE::MUTANT)
            {
                _state = EnemyState::DEAD;
                _death_timer = 0; // Will be set to 1 in update()
            }
            // Stop all movement when dead
            _dx = 0;
            _dy = 0;
            _target_dx = 0;
            _target_dy = 0;
            _attacking = false;
            _attack_timer = 0;
        }

        return true;
    }

    void Enemy::_apply_knockback(bn::fixed dx, bn::fixed dy)
    {
        static constexpr int KNOCKBACK_DURATION = 10;
        const bn::fixed KNOCKBACK_STRENGTH = 2.5;
        _knockback_dx = dx * KNOCKBACK_STRENGTH;
        _knockback_dy = dy * KNOCKBACK_STRENGTH;
        _knockback_timer = KNOCKBACK_DURATION; // Knockback duration
        _stunned = true;
    }

    bool Enemy::damage_from_left(int damage)
    {
        if (_take_damage(damage))
        {
            _apply_knockback(1.0, -0.5); // Knock to the right and slightly up
            return true;
        }
        return false;
    }

    bool Enemy::damage_from_right(int damage)
    {
        if (_take_damage(damage))
        {
            _apply_knockback(-1.0, -0.5); // Knock to the left and slightly up
            return true;
        }
        return false;
    }

    bool Enemy::is_hit(Hitbox /*attack*/)
    {
        return false;
    }

    bool Enemy::is_vulnerable()
    {
        return !_invulnerable;
    }

    void Enemy::set_visible(bool visibility)
    {
        if (_sprite.has_value())
        {
            _sprite->set_visible(visibility);
        }
    }

    void Enemy::teleport()
    {
        // TODO: Implement teleport
    }

    bool Enemy::spotted_player()
    {
        return _spotted_player;
    }

    int Enemy::hp()
    {
        return _hp;
    }

    ENEMY_TYPE Enemy::type()
    {
        return _type;
    }

    void Enemy::_update_animation()
    {
        // Only apply advanced animation system to spearguard enemies (MUTANT type)
        // Other enemy types use simple default animations
        if (_type != ENEMY_TYPE::MUTANT)
        {
            // Simple animation for non-spearguard enemies - just update existing action
            if (_sprite.has_value() && _action.has_value())
            {
                // Keep using the basic animation set up in constructor
                return;
            }
        }

        // Advanced animation system for spearguard enemies only
        // Determine which animation state should be active
        EnemyState target_animation_state = EnemyState::IDLE;
        
        if (_dead)
        {
            target_animation_state = EnemyState::DEAD;
        }
        else if (_attacking && _attack_timer > 0)
        {
            target_animation_state = EnemyState::ATTACK;
        }
        else if (_state == EnemyState::FOLLOW || _state == EnemyState::WALK)
        {
            // Only show run animation if actually moving
            if (bn::abs(_dx) > 0.1 || bn::abs(_dy) > 0.1)
            {
                target_animation_state = EnemyState::WALK; // Map WALK to RUN animation
            }
            else
            {
                target_animation_state = EnemyState::IDLE;
            }
        }
        else
        {
            target_animation_state = EnemyState::IDLE;
        }

        // If animation state changed, create new animation
        if (target_animation_state != _current_animation_state)
        {
            _current_animation_state = target_animation_state;
            
            if (_sprite.has_value())
            {
                // Get frame range for current animation state
                AnimationFrameRange frame_range = IDLE_FRAMES;
                switch (_current_animation_state)
                {
                    case EnemyState::IDLE:
                        frame_range = IDLE_FRAMES;
                        break;
                    case EnemyState::WALK:
                        frame_range = RUN_FRAMES;
                        break;
                    case EnemyState::ATTACK:
                        frame_range = ATTACK_FRAMES;
                        break;
                    case EnemyState::DEAD:
                        frame_range = DEAD_FRAMES;
                        break;
                    default:
                        frame_range = IDLE_FRAMES;
                        break;
                }

                // For sprites with limited frames, map to available frames
                // Most existing sprites have 4 frames (0-3), so we map the ranges accordingly
                int available_frames = 4; // Current spearguard sprite has 4 frames
                
                if (frame_range.end >= available_frames)
                {
                    // Map animation to available frame range
                    if (_current_animation_state == EnemyState::DEAD)
                    {
                        // For death, use all available frames in sequence
                        _action = bn::create_sprite_animate_action_forever(
                            *_sprite,
                            frame_range.speed,
                            bn::sprite_items::spearguard.tiles_item(),
                            0, 1, 2, 3
                        );
                    }
                    else if (_current_animation_state == EnemyState::ATTACK)
                    {
                        // For attack, use frames 2,3 (later frames for more aggressive look)
                        _action = bn::create_sprite_animate_action_forever(
                            *_sprite,
                            frame_range.speed,
                            bn::sprite_items::spearguard.tiles_item(),
                            2, 3
                        );
                    }
                    else if (_current_animation_state == EnemyState::WALK)
                    {
                        // For run/walk, use all frames for movement
                        _action = bn::create_sprite_animate_action_forever(
                            *_sprite,
                            frame_range.speed,
                            bn::sprite_items::spearguard.tiles_item(),
                            0, 1, 2, 3
                        );
                    }
                    else // IDLE
                    {
                        // For idle, use frames 0,1 for subtle animation
                        _action = bn::create_sprite_animate_action_forever(
                            *_sprite,
                            frame_range.speed,
                            bn::sprite_items::spearguard.tiles_item(),
                            0, 1
                        );
                    }
                }
                else
                {
                    // Sprite has enough frames, use exact frame range
                    // This would be used for sprites that actually have 31+ frames
                    // For now, fall back to basic animation
                    _action = bn::create_sprite_animate_action_forever(
                        *_sprite,
                        frame_range.speed,
                        bn::sprite_items::spearguard.tiles_item(),
                        0, 1, 2, 3
                    );
                }
            }
        }
    }
}
