#include "fe_enemy.h"
#include "fe_enemy_states.h"
#include "fe_enemy_state_machine.h"
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
#include "bn_memory.h"

#include "bn_sprite_items_spearguard.h"

#include "fe_level.h"
#include "fe_enemy_type.h"
#include "fe_collision.h"

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

    Enemy::Enemy(int x, int y, bn::camera_ptr camera, bn::regular_bg_ptr map, ENEMY_TYPE type, int hp) : Entity(bn::fixed_point(x, y)),
                                                                                                         _camera(camera),
                                                                                                         _type(type),
                                                                                                         _dir(0),
                                                                                                         _hp(hp),
                                                                                                         _map(map),
                                                                                                         _map_cells(map.map().cells_ref().value())
    {
        // Create sprite using builder based on enemy type
        bn::sprite_builder builder(bn::sprite_items::spearguard); // Default to spearguard

        // Select appropriate sprite based on enemy type
        switch (_type)
        {
        case ENEMY_TYPE::SPEARGUARD:
            builder = bn::sprite_builder(bn::sprite_items::spearguard);
            break;
        case ENEMY_TYPE::SLIME:
            builder = bn::sprite_builder(bn::sprite_items::spearguard); // Use spearguard for slime until proper slime sprite is available
            break;
        case ENEMY_TYPE::MUTANT:
            builder = bn::sprite_builder(bn::sprite_items::spearguard); // Use spearguard for mutant until proper mutant sprite is available
            break;
        default:
            builder = bn::sprite_builder(bn::sprite_items::spearguard);
            break;
        }
        builder.set_position(pos());
        builder.set_bg_priority(1);

        _sprite = builder.build();
        if (!_sprite.has_value())
        {
            return;
        }

        set_camera(_camera);

        _hitbox = Hitbox(pos().x() - 4, pos().y() - 4, 8, 8);

        // Store original position for spearguards to return to
        if (_type == ENEMY_TYPE::SPEARGUARD)
        {
            _original_position = pos();
        }

        // Setup initial animation based on enemy type
        if (_type == ENEMY_TYPE::SPEARGUARD)
        {
            // Start with Idle animation (frames 0-5)
            _action = bn::create_sprite_animate_action_forever(
                *_sprite,
                12, // Slower animation speed for idle
                bn::sprite_items::spearguard.tiles_item(),
                0, 1, 2, 3, 4, 5 // Idle frames
            );
            _current_animation = AnimationState::IDLE;
        }
        else
        {
            // Default animation for other enemy types
            _action = bn::create_sprite_animate_action_forever(
                *_sprite,
                8,
                bn::sprite_items::spearguard.tiles_item(),
                0, 1, 2, 3);
        }

        // Initialize the state machine with IdleState
        // This provides a clean starting state for all enemies
        bn::unique_ptr<IdleState> initial_state = bn::make_unique<IdleState>();
        _state_machine.initialize(bn::move(initial_state));
    }

    Enemy::Enemy(Enemy &&other) noexcept
        : Entity(bn::move(other)), // Move base class
          _movement(bn::move(other._movement)),
          _state_machine(bn::move(other._state_machine)),
          _state_timer(other._state_timer),
          _state_duration(other._state_duration),
          _target_dx(other._target_dx),
          _target_dy(other._target_dy),
          _dx(other._dx),
          _dy(other._dy),
          _camera(other._camera),
          _type(other._type),
          _dir(other._dir),
          _hp(other._hp),
          _direction_timer(other._direction_timer),
          _invulnerable(other._invulnerable),
          _dead(other._dead),
          _grounded(other._grounded),
          _inv_timer(other._inv_timer),
          _stunned(other._stunned),
          _death_timer(other._death_timer),
          _knockback_dx(other._knockback_dx),
          _knockback_dy(other._knockback_dy),
          _knockback_timer(other._knockback_timer),
          _sound_timer(other._sound_timer),
          _spotted_player(other._spotted_player),
          _action(bn::move(other._action)),
          _current_animation(other._current_animation),
          _attack_timer(other._attack_timer),
          _original_position(other._original_position),
          _returning_to_post(other._returning_to_post),
          _target(other._target),
          _target_locked(other._target_locked),
          _map(other._map),
          _map_cells(other._map_cells),
          _level(other._level)
    {
        // Reset moved-from object to a valid state
        other._hp = 0;
        other._dead = true;
    }

    Enemy &Enemy::operator=(Enemy &&other) noexcept
    {
        if (this != &other)
        {
            // Move base class
            Entity::operator=(bn::move(other));

            // Move all members
            _movement = bn::move(other._movement);
            _state_machine = bn::move(other._state_machine);
            _state_timer = other._state_timer;
            _state_duration = other._state_duration;
            _target_dx = other._target_dx;
            _target_dy = other._target_dy;
            _dx = other._dx;
            _dy = other._dy;
            _camera = other._camera;
            _type = other._type;
            _dir = other._dir;
            _hp = other._hp;
            _direction_timer = other._direction_timer;
            _invulnerable = other._invulnerable;
            _dead = other._dead;
            _grounded = other._grounded;
            _inv_timer = other._inv_timer;
            _stunned = other._stunned;
            _death_timer = other._death_timer;
            _knockback_dx = other._knockback_dx;
            _knockback_dy = other._knockback_dy;
            _knockback_timer = other._knockback_timer;
            _sound_timer = other._sound_timer;
            _spotted_player = other._spotted_player;
            _action = bn::move(other._action);
            _current_animation = other._current_animation;
            _attack_timer = other._attack_timer;
            _original_position = other._original_position;
            _returning_to_post = other._returning_to_post;
            _target = other._target;
            _target_locked = other._target_locked;
            _map = other._map;
            _map_cells = other._map_cells;
            _level = other._level;

            // Reset moved-from object to a valid state
            other._hp = 0;
            other._dead = true;
        }
        return *this;
    }

    void Enemy::update_hitbox()
    {
        // Adjust hitbox position to account for center-based sprite positioning
        _hitbox.set_x(pos().x() - 4); // Adjust for smaller hitbox
        _hitbox.set_y(pos().y() - 4);
    }

    void Enemy::update(bn::fixed_point player_pos, const Level &level, bool player_listening)
    {
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

                // Transition to stunned state for recovery
                bn::unique_ptr<StunnedState> stunned_state = bn::make_unique<StunnedState>();
                _state_machine.transition_to(*this, bn::move(stunned_state));
            }

            // Update hitbox and return early - don't process normal AI during knockback
            update_hitbox();
            return;
        }

        // Don't update state machine if dead - let death animation play
        if (!_dead)
        {
            // Update the state machine
            _state_machine.update(*this, player_pos, level, player_listening);

            // Smoothly interpolate _dx/_dy to _target_dx/_target_dy
            const bn::fixed lerp = 0.1;
            _dx += (_target_dx - _dx) * lerp;
            _dy += (_target_dy - _dy) * lerp;

            // Update movement component
            _movement.set_velocity(bn::fixed_point(_dx, _dy));
            _movement.update();

            // Update position with collision checking
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

            if (fe::Collision::check_hitbox_collision_with_level(_hitbox, new_pos, check_direction))
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
                    can_move_x = fe::Collision::check_hitbox_collision_with_level(_hitbox, x_pos, x_dir);
                }

                // Check Y movement only
                if (_dy != 0)
                {
                    bn::fixed_point y_pos(pos().x(), pos().y() + _dy);
                    fe::directions y_dir = _dy > 0 ? fe::directions::down : fe::directions::up;
                    can_move_y = fe::Collision::check_hitbox_collision_with_level(_hitbox, y_pos, y_dir);
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
        }
        else
        {
            // Dead enemies stop moving immediately
            _dx = 0;
            _dy = 0;
            _target_dx = 0;
            _target_dy = 0;
            _movement.set_velocity(bn::fixed_point(0, 0));

            // Count down death timer
            if (_death_timer > 0)
            {
                _death_timer--;
            }
        }

        // Handle invulnerability timer (but not for dead enemies)
        if (_invulnerable && !_dead)
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
        else if (_dead && _sprite.has_value())
        {
            // Make sure dead enemies are always visible
            _sprite->set_visible(true);
        }

        // Update spearguard animations
        _update_spearguard_animation();

        // Update sprite if available
        if (_sprite.has_value())
        {
            _sprite->set_position(pos());
            _sprite->set_horizontal_flip(_dx < 0);
            if (_action.has_value())
            {
                // Only update action if it's not done (prevents error with completed "once" animations)
                if (!_action->done())
                {
                    _action->update();
                }
            }
        }

        // Update hitbox
        update_hitbox();
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
            _death_timer = DEATH_ANIMATION_DURATION; // Start death timer
        }

        return true;
    }

    void Enemy::_apply_knockback(bn::fixed dx, bn::fixed dy)
    {
        const bn::fixed KNOCKBACK_STRENGTH = 2.5;
        _knockback_dx = dx * KNOCKBACK_STRENGTH;
        _knockback_dy = dy * KNOCKBACK_STRENGTH;
        _knockback_timer = KNOCKBACK_DURATION; // Use class constant
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

    bool Enemy::is_ready_for_removal()
    {
        return _dead && _death_timer <= 0;
    }

    void Enemy::_update_spearguard_animation()
    {
        if (_type != ENEMY_TYPE::SPEARGUARD || !_sprite.has_value())
        {
            return;
        }

        AnimationState desired_animation = AnimationState::IDLE;

        // Determine desired animation based on enemy state
        if (_dead)
        {
            desired_animation = AnimationState::DEAD;
        }
        else if (_attack_timer > 0)
        {
            // Always prioritize attack animation if attack timer is active
            desired_animation = AnimationState::ATTACK;
            _attack_timer--;
        }
        else if (_state_machine.get_current_state_id() == EnemyStateId::CHASE ||
                 _state_machine.get_current_state_id() == EnemyStateId::PATROL ||
                 _state_machine.get_current_state_id() == EnemyStateId::RETURN_TO_POST)
        {
            // Only run if not attacking
            desired_animation = AnimationState::RUN;
        }
        else
        {
            // Default to idle
            desired_animation = AnimationState::IDLE;
        }

        // Only update animation if it changed
        if (desired_animation != _current_animation)
        {
            _current_animation = desired_animation;

            switch (_current_animation)
            {
            case AnimationState::IDLE:
                _action = bn::create_sprite_animate_action_forever(
                    *_sprite,
                    12, // Slower for idle
                    bn::sprite_items::spearguard.tiles_item(),
                    0, 1, 2, 3, 4, 5 // Idle frames 0-5
                );
                break;

            case AnimationState::RUN:
                _action = bn::create_sprite_animate_action_forever(
                    *_sprite,
                    8, // Faster for running
                    bn::sprite_items::spearguard.tiles_item(),
                    6, 7, 8, 9 // Run frames 6-9
                );
                break;

            case AnimationState::ATTACK:
                _action = bn::create_sprite_animate_action_forever(
                    *_sprite,
                    6, // Fast for attack
                    bn::sprite_items::spearguard.tiles_item(),
                    10, 11, 12, 13, 14 // Attack frames 10-14
                );
                break;

            case AnimationState::DEAD:
                // Use all available death frames (15-30) and play only once
                _action = bn::create_sprite_animate_action_once(
                    *_sprite,
                    8, // Medium speed for death
                    bn::sprite_items::spearguard.tiles_item(),
                    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30 // All dead frames 15-30
                );
                break;

            default:
                // Default case to fix warning
                break;
            }
        }
    }

    Hitbox Enemy::get_attack_hitbox() const
    {
        // For spearguards during attack, extend the hitbox to represent the spear reach
        if (_type == ENEMY_TYPE::SPEARGUARD && is_attacking())
        {
            bn::fixed_point enemy_pos = pos();

            // Determine the direction the spearguard is facing
            bool facing_left = false;
            if (_sprite.has_value())
            {
                facing_left = _sprite->horizontal_flip();
            }

            // Create an extended hitbox in the direction the spearguard is facing
            // Original hitbox is 8x8, we'll extend it by SPEAR_REACH pixels in the facing direction
            // for a moderate spear reach that matches the visual sprite
            constexpr bn::fixed HITBOX_WIDTH = 8;
            constexpr bn::fixed HITBOX_HEIGHT = 8;

            if (facing_left)
            {
                // Extend hitbox to the left
                return Hitbox(enemy_pos.x() - (HITBOX_WIDTH / 2 + SPEAR_REACH),
                              enemy_pos.y() - HITBOX_HEIGHT / 2,
                              HITBOX_WIDTH + SPEAR_REACH,
                              HITBOX_HEIGHT);
            }
            else
            {
                // Extend hitbox to the right
                return Hitbox(enemy_pos.x() - HITBOX_WIDTH / 2,
                              enemy_pos.y() - HITBOX_HEIGHT / 2,
                              HITBOX_WIDTH + SPEAR_REACH,
                              HITBOX_HEIGHT);
            }
        }

        // For all other cases, return the normal hitbox
        return get_hitbox();
    }

    bool Enemy::is_attacking() const
    {
        if (_type != ENEMY_TYPE::SPEARGUARD || _attack_timer <= 0 || _current_animation != AnimationState::ATTACK)
        {
            return false;
        }

        // Extend attack hitbox during most of the spear jab animation
        // Attack animation frames: 10, 11, 12, 13, 14 (5 frames total)
        // Animation speed: 6 frames per sprite frame
        // Total attack duration: 60 frames

        constexpr int ATTACK_FRAMES_PER_SPRITE = 6;
        constexpr int TOTAL_ATTACK_FRAMES = 5;

        // Calculate which animation frame we're currently on
        int frames_elapsed = ATTACK_DURATION - _attack_timer;
        int current_anim_frame = (frames_elapsed / ATTACK_FRAMES_PER_SPRITE) % TOTAL_ATTACK_FRAMES; // 5 attack frames total

        // Delay damage until frames 2 and 3 (when spear is fully extending and extended)
        // This gives a wind-up period before the spear can actually hurt the player
        return current_anim_frame >= 3 && current_anim_frame <= 3;
    }
}
