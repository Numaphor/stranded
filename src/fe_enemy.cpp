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

namespace fe
{
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

    Enemy::Enemy(int x, int y, bn::camera_ptr camera, bn::regular_bg_ptr map, ENEMY_TYPE type, int hp) : _pos(x, y),
                                                                                                         _hitbox(x - 4, y - 4, 8, 8), // Reduce hitbox size to 8x8, centered on the sprite
                                                                                                         _camera(camera),
                                                                                                         _type(type),
                                                                                                         _dir(0),
                                                                                                         _hp(hp),
                                                                                                         _map(map),
                                                                                                         _map_cells(map.map().cells_ref().value())
    {
        // Create sprite using builder with safe defaults
        bn::sprite_builder builder(bn::sprite_items::spearguard);
        builder.set_position(_pos);
        builder.set_bg_priority(1);

        _sprite = builder.build();
        if (!_sprite.has_value())
        {
            return;
        }

        _sprite->set_camera(_camera);

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
        _hitbox.set_x(_pos.x() - 4); // Adjust for smaller hitbox
        _hitbox.set_y(_pos.y() - 4);
    }

    void Enemy::update(bn::fixed_point player_pos, const Level &level, bool player_listening)
    {
        // Handle knockback first
        if (_knockback_timer > 0)
        {
            _knockback_timer--;

            // Apply knockback movement
            _pos.set_x(_pos.x() + _knockback_dx);
            _pos.set_y(_pos.y() + _knockback_dy);

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
        bn::fixed dist_x = player_pos.x() - _pos.x();
        bn::fixed dist_y = player_pos.y() - _pos.y();
        bn::fixed dist_sq = dist_x * dist_x + dist_y * dist_y;
        const bn::fixed follow_dist_sq = 48 * 48;   // 6 tiles squared
        const bn::fixed unfollow_dist_sq = 64 * 64; // 8 tiles squared

        // Aggro logic - enemies can't detect player when they're listening to NPCs
        if (!player_listening && _state != EnemyState::FOLLOW && dist_sq <= follow_dist_sq)
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

        // Update position
        bn::fixed new_x = _pos.x() + _dx;
        bn::fixed new_y = _pos.y() + _dy;
        bn::fixed_point new_pos(new_x, new_y);

        // --- Robust collision check with proper direction ---
        bn::fixed_point points[4];

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
            _pos = new_pos;
            update_hitbox();
        }
        else
        {
            // Try to slide along walls by checking individual axis movement
            bool can_move_x = true;
            bool can_move_y = true;

            // Check X movement only
            if (_dx != 0)
            {
                bn::fixed_point x_pos(_pos.x() + _dx, _pos.y());
                fe::directions x_dir = _dx > 0 ? fe::directions::right : fe::directions::left;
                can_move_x = fe::Collision::check_hitbox_collision_with_level(_hitbox, x_pos, x_dir, level);
            }

            // Check Y movement only
            if (_dy != 0)
            {
                bn::fixed_point y_pos(_pos.x(), _pos.y() + _dy);
                fe::directions y_dir = _dy > 0 ? fe::directions::down : fe::directions::up;
                can_move_y = fe::Collision::check_hitbox_collision_with_level(_hitbox, y_pos, y_dir, level);
            }

            // Apply movement on valid axes
            if (can_move_x)
            {
                _pos.set_x(_pos.x() + _dx);
            }
            else
            {
                _dx = 0;
            }

            if (can_move_y)
            {
                _pos.set_y(_pos.y() + _dy);
            }
            else
            {
                _dy = 0;
            }

            update_hitbox();
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
            _sprite->set_position(_pos);
            _sprite->set_horizontal_flip(_dx < 0);
            if (_action.has_value())
            {
                _action->update();
            }
        }
    }

    void Enemy::set_pos(bn::fixed_point pos)
    {
        _pos = pos;
        update_hitbox();
        if (_sprite.has_value())
        {
            _sprite->set_position(pos);
        }
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
}
