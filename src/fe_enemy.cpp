#include "fe_enemy.h"
#include "fe_enemy_states.h"
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
        if (_type == ENEMY_TYPE::SPEARGUARD) {
            _original_position = pos();
        }

        // Setup initial animation based on enemy type
        if (_type == ENEMY_TYPE::SPEARGUARD) {
            // Start with Idle animation (frames 0-5)
            _action = bn::create_sprite_animate_action_forever(
                *_sprite,
                12, // Slower animation speed for idle
                bn::sprite_items::spearguard.tiles_item(),
                0, 1, 2, 3, 4, 5 // Idle frames
            );
            _current_animation = AnimationState::IDLE;
        } else {
            // Default animation for other enemy types
            _action = bn::create_sprite_animate_action_forever(
                *_sprite,
                8,
                bn::sprite_items::spearguard.tiles_item(),
                0, 1, 2, 3
            );
        }
        
        // Initialize the new state machine with IdleState
        // Note: The new state machine is only used if _use_new_state_machine is set to true
        // This allows for gradual migration and testing
        bn::unique_ptr<IdleState> initial_state = bn::make_unique<IdleState>();
        _state_machine.initialize(bn::move(initial_state));
    }

    Enemy::Enemy(Enemy&& other) noexcept
        : Entity(bn::move(other)), // Move base class
          _movement(bn::move(other._movement)),
          _state(other._state),
          _state_machine(bn::move(other._state_machine)),
          _use_new_state_machine(other._use_new_state_machine),
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
        other._use_new_state_machine = false;
    }

    Enemy& Enemy::operator=(Enemy&& other) noexcept
    {
        if (this != &other)
        {
            // Move base class
            Entity::operator=(bn::move(other));
            
            // Move all members
            _movement = bn::move(other._movement);
            _state = other._state;
            _state_machine = bn::move(other._state_machine);
            _use_new_state_machine = other._use_new_state_machine;
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
            other._use_new_state_machine = false;
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
        // Migration helper: use new state machine if enabled
        if (_use_new_state_machine)
        {
            update_with_new_state_machine(player_pos, level, player_listening);
            return;
        }
        
        // Original implementation continues below for backward compatibility
        
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
        const bn::fixed follow_dist_sq = 48 * 48;   // 6 tiles squared
        const bn::fixed unfollow_dist_sq = 64 * 64; // 8 tiles squared

        // Aggro logic - enemies can't detect player when they're listening to NPCs
        if (!player_listening && _state != EnemyState::FOLLOW && dist_sq <= follow_dist_sq)
        {
            _state = EnemyState::FOLLOW;
            _state_timer = 0;
            _returning_to_post = false; // Cancel return if player comes back in range
        }
        else if (_state == EnemyState::FOLLOW && (dist_sq > unfollow_dist_sq || player_listening))
        {
            // For spearguards, start returning to post instead of just going idle
            if (_type == ENEMY_TYPE::SPEARGUARD)
            {
                _returning_to_post = true;
                _state = EnemyState::WALK; // Use walk state for returning
            }
            else
            {
                _state = EnemyState::IDLE;
                _target_dx = 0;
                _target_dy = 0;
                // Random idle duration
                _state_duration = 20 + (random.get() % 40);
            }
            _state_timer = 0;
        }

        // State logic
        if (_state == EnemyState::FOLLOW)
        {
            // Check if close enough to attack (spearguards only)
            if (_type == ENEMY_TYPE::SPEARGUARD && dist_sq <= 32 * 32 && _attack_timer <= 0) { // 4 tiles attack range
                _attack_timer = ATTACK_DURATION;
                _target_dx = 0;
                _target_dy = 0;
            } else {
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
        }
        else
        {
            // Different behavior for spearguards vs other enemies
            if (_type == ENEMY_TYPE::SPEARGUARD)
            {
                // Check if spearguard is returning to post
                if (_returning_to_post)
                {
                    // Calculate distance to original position
                    bn::fixed dist_to_post_x = _original_position.x() - pos().x();
                    bn::fixed dist_to_post_y = _original_position.y() - pos().y();
                    bn::fixed dist_to_post_sq = dist_to_post_x * dist_to_post_x + dist_to_post_y * dist_to_post_y;
                    
                    // If close enough to original position, stop and go idle
                    if (dist_to_post_sq <= RETURN_THRESHOLD * RETURN_THRESHOLD)
                    {
                        _returning_to_post = false;
                        _state = EnemyState::IDLE;
                        _target_dx = 0;
                        _target_dy = 0;
                        // Snap to exact original position
                        set_position(_original_position);
                    }
                    else
                    {
                        // Set state to WALK when returning to post
                        _state = EnemyState::WALK;
                        // Move toward original position
                        bn::fixed speed = 0.25; // Slower return speed
                        bn::fixed len = bn::sqrt(dist_to_post_sq);
                        if (len > 0.1)
                        {
                            _target_dx = (dist_to_post_x / len) * speed;
                            _target_dy = (dist_to_post_y / len) * speed;
                        }
                        else
                        {
                            _target_dx = 0;
                            _target_dy = 0;
                        }
                    }
                }
                else
                {
                    // Spearguards stand still and wait - no wandering
                    _state = EnemyState::IDLE;
                    _target_dx = 0;
                    _target_dy = 0;
                }
            }
            else
            {
                // Other enemies wander/idle as before
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

        // Update spearguard animations
        _update_spearguard_animation();
        
        // Update sprite if available
        if (_sprite.has_value())
        {
            _sprite->set_position(pos());
            _sprite->set_horizontal_flip(_dx < 0);
            if (_action.has_value())
            {
                _action->update();
            }
        }
    }

    void Enemy::update_with_new_state_machine(bn::fixed_point player_pos, const Level &level, bool player_listening)
    {
        // Handle knockback first (same as before)
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

        // Update the state machine
        _state_machine.update(*this, player_pos, level, player_listening);

        // Smoothly interpolate _dx/_dy to _target_dx/_target_dy
        const bn::fixed lerp = 0.1;
        _dx += (_target_dx - _dx) * lerp;
        _dy += (_target_dy - _dy) * lerp;
        
        // Update movement component
        _movement.set_velocity(bn::fixed_point(_dx, _dy));
        _movement.update();

        // Update position with collision checking (same as before)
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

        // Handle invulnerability timer (same as before)
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

        // Update spearguard animations
        _update_spearguard_animation();
        
        // Update sprite if available (same as before)
        if (_sprite.has_value())
        {
            _sprite->set_position(pos());
            _sprite->set_horizontal_flip(_dx < 0);
            if (_action.has_value())
            {
                _action->update();
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

    void Enemy::_update_spearguard_animation()
    {
        if (_type != ENEMY_TYPE::SPEARGUARD || !_sprite.has_value()) {
            return;
        }

        AnimationState desired_animation = AnimationState::IDLE;
        
        // Determine desired animation based on enemy state
        if (_dead) {
            desired_animation = AnimationState::DEAD;
        } else if (_attack_timer > 0) {
            // Always prioritize attack animation if attack timer is active
            desired_animation = AnimationState::ATTACK;
            _attack_timer--;
        } else if (_state == EnemyState::FOLLOW || _state == EnemyState::WALK) {
            // Only run if not attacking
            desired_animation = AnimationState::RUN;
        } else {
            // Default to idle
            desired_animation = AnimationState::IDLE;
        }

        // Only update animation if it changed
        if (desired_animation != _current_animation) {
            _current_animation = desired_animation;
            
            switch (_current_animation) {
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
                    _action = bn::create_sprite_animate_action_forever(
                        *_sprite,
                        10, // Medium speed for death
                        bn::sprite_items::spearguard.tiles_item(),
                        15, 16, 17, 18, 19, 20, 21, 22, 23, 24 // Dead frames 15-24 (reduced to 10 frames)
                    );
                    break;
                    
                default:
                    // Default case to fix warning
                    break;
            }
        }
    }
}
