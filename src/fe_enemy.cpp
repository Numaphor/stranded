#include "fe_enemy.h"
#include "fe_constants.h"
#include "fe_enemy_states.h"
#include "fe_enemy_state_machine.h"
#include "bn_fixed_point.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_shape_size.h"
#include "bn_sprite_items_healthbar_enemy.h"
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
    constexpr int HEALTHBAR_Z_ORDER = -1000; // Z-order for healthbar sprite
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
                                                                                                         _max_hp(hp),
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

        // Initialize healthbar
        _health_bar_sprite = bn::sprite_items::healthbar_enemy.create_sprite(pos().x(), pos().y() - 20, 0);
        _health_bar_sprite->set_camera(_camera);
        _health_bar_sprite->set_bg_priority(3);
        _health_bar_sprite->set_z_order(HEALTHBAR_Z_ORDER);
        _update_health_bar();
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
          _health_bar_sprite(bn::move(other._health_bar_sprite)),
          _max_hp(other._max_hp),
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
            _health_bar_sprite = bn::move(other._health_bar_sprite);
            _max_hp = other._max_hp;
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
            directions check_direction = directions::down; // Default
            if (bn::abs(_dx) > bn::abs(_dy))
            {
                // Moving horizontally
                check_direction = _dx > 0 ? directions::right : directions::left;
            }
            else
            {
                // Moving vertically
                check_direction = _dy > 0 ? directions::down : directions::up;
            }

            // Note: Enemy collision with zones could be handled here if needed
            // For now, enemies use their own collision logic separate from player zones
            set_position(new_pos);
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

        // Update healthbar position
        _update_health_bar_position();

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

        // Spearguards become aggroed when shot - they will chase indefinitely
        if (_type == ENEMY_TYPE::SPEARGUARD)
        {
            _aggroed = true;
        }

        // Update healthbar
        _update_health_bar();

        if (_hp <= 0)
        {
            _dead = true;
            _death_timer = ENEMY_DEATH_ANIMATION_DURATION; // Start death timer
        }

        return true;
    }

    void Enemy::_apply_knockback(bn::fixed dx, bn::fixed dy)
    {
        _knockback_dx = dx * ENEMY_KNOCKBACK_STRENGTH;
        _knockback_dy = dy * ENEMY_KNOCKBACK_STRENGTH;
        _knockback_timer = ENEMY_KNOCKBACK_DURATION; // Use class constant
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

    void Enemy::_update_health_bar()
    {
        if (_health_bar_sprite.has_value())
        {
            // Calculate healthbar frame for 3-slot system (frames are in reverse order)
            // Frame 0 = full (3 slots), Frame 1 = 2 slots, Frame 2 = 1 slot, Frame 3 = empty (0 health)
            int frame;
            if (_hp <= 0 || _dead)
            {
                frame = 3; // Empty
            }
            else if (_hp >= _max_hp)
            {
                frame = 0; // Full - 3 slots
            }
            else
            {
                // Map HP to slots and invert: for 3 max HP -> 3 HP = frame 0, 2 HP = frame 1, 1 HP = frame 2
                int health_slots = (_hp * 3) / _max_hp;
                if (health_slots == 0 && _hp > 0)
                    health_slots = 1;     // Ensure at least 1 slot if HP > 0
                frame = 3 - health_slots; // Invert the frame order
            }
            
            // Recreate the sprite tiles with the correct frame
            bn::sprite_tiles_ptr new_tiles = bn::sprite_items::healthbar_enemy.tiles_item().create_tiles(frame);
            _health_bar_sprite->set_tiles(bn::move(new_tiles));
            _health_bar_sprite->set_visible(true);
        }
    }

    void Enemy::_update_health_bar_position()
    {
        if (_health_bar_sprite.has_value())
        {
            // Position healthbar above the enemy
            _health_bar_sprite->set_position(pos().x() - 3, pos().y() - 12);
        }
    }
}
#include "fe_enemy_state_machine.h"
#include "fe_enemy.h"
#include "bn_memory.h"

namespace fe
{
    EnemyStateMachine::EnemyStateMachine() 
        : _current_state(nullptr), _current_state_id(EnemyStateId::IDLE), _state_timer(0)
    {
    }

    EnemyStateMachine::EnemyStateMachine(EnemyStateMachine&& other) noexcept
        : _current_state(bn::move(other._current_state)), 
          _current_state_id(other._current_state_id), 
          _state_timer(other._state_timer)
    {
        // Reset the moved-from object
        other._current_state_id = EnemyStateId::IDLE;
        other._state_timer = 0;
    }

    EnemyStateMachine& EnemyStateMachine::operator=(EnemyStateMachine&& other) noexcept
    {
        if (this != &other)
        {
            _current_state = bn::move(other._current_state);
            _current_state_id = other._current_state_id;
            _state_timer = other._state_timer;
            
            // Reset the moved-from object
            other._current_state_id = EnemyStateId::IDLE;
            other._state_timer = 0;
        }
        return *this;
    }

    void EnemyStateMachine::initialize(bn::unique_ptr<EnemyState> initial_state)
    {
        if (initial_state)
        {
            _current_state = bn::move(initial_state);
            _current_state_id = static_cast<EnemyStateId>(_current_state->get_state_id());
            _state_timer = 0;
        }
    }

    void EnemyStateMachine::update(Enemy& enemy, bn::fixed_point player_pos, const Level& level, bool player_listening)
    {
        if (_current_state)
        {
            _current_state->update(enemy, player_pos, level, player_listening);
            _state_timer++;
        }
    }

    void EnemyStateMachine::transition_to(Enemy& enemy, bn::unique_ptr<EnemyState> new_state)
    {
        if (!new_state)
            return;

        // Exit current state
        if (_current_state)
        {
            _current_state->exit(enemy);
        }

        // Transition to new state
        _current_state = bn::move(new_state);
        _current_state_id = static_cast<EnemyStateId>(_current_state->get_state_id());
        _state_timer = 0;

        // Enter new state
        _current_state->enter(enemy);
    }
}
#include "fe_enemy_states.h"
#include "fe_constants.h"
#include "fe_enemy.h"
#include "fe_enemy_type.h"
#include "fe_level.h"
#include "fe_enemy_state_machine.h"
#include "bn_log.h"
#include "bn_random.h"
#include "bn_memory.h"
#include "bn_math.h"

namespace fe
{
    // IdleState implementation
    void IdleState::enter(Enemy &enemy)
    {
        // Stop movement when entering idle
        enemy._target_dx = 0;
        enemy._target_dy = 0;
        enemy._dx = 0;
        enemy._dy = 0;
    }

    void IdleState::update(Enemy &enemy, bn::fixed_point player_pos, const Level &level, bool player_listening)
    {
        // Calculate distance to player for state transitions
        bn::fixed dist_x = player_pos.x() - enemy.pos().x();
        bn::fixed dist_y = player_pos.y() - enemy.pos().y();
        bn::fixed dist_sq = dist_x * dist_x + dist_y * dist_y;
        const bn::fixed follow_dist_sq = 48 * 48; // 6 tiles squared

        // Check if player should be chased (only if not listening to NPCs)
        if (!player_listening && dist_sq <= follow_dist_sq)
        {
            // Transition to chase state
            bn::unique_ptr<ChaseState> chase_state = bn::make_unique<ChaseState>();
            enemy._state_machine.transition_to(enemy, bn::move(chase_state));
            return;
        }

        // Check if idle time is up and should start patrolling (for non-guard enemies)
        if (enemy.type() != ENEMY_TYPE::SPEARGUARD && enemy._state_machine.get_state_timer() >= _idle_duration)
        {
            // Transition to patrol state
            bn::unique_ptr<PatrolState> patrol_state = bn::make_unique<PatrolState>();
            enemy._state_machine.transition_to(enemy, bn::move(patrol_state));
            return;
        }

        // Stay idle - maintain zero velocity
        enemy._target_dx = 0;
        enemy._target_dy = 0;
    }

    void IdleState::exit(Enemy &enemy)
    {
        // No cleanup needed for idle state
    }

    // PatrolState implementation
    void PatrolState::enter(Enemy &enemy)
    {
        _direction_set = false;
        _target_dx = 0;
        _target_dy = 0;
    }

    void PatrolState::update(Enemy &enemy, bn::fixed_point player_pos, const Level &level, bool player_listening)
    {
        // Calculate distance to player for state transitions
        bn::fixed dist_x = player_pos.x() - enemy.pos().x();
        bn::fixed dist_y = player_pos.y() - enemy.pos().y();
        bn::fixed dist_sq = dist_x * dist_x + dist_y * dist_y;
        const bn::fixed follow_dist_sq = 48 * 48; // 6 tiles squared

        // Check if player should be chased (only if not listening to NPCs)
        if (!player_listening && dist_sq <= follow_dist_sq)
        {
            // Transition to chase state
            bn::unique_ptr<ChaseState> chase_state = bn::make_unique<ChaseState>();
            enemy._state_machine.transition_to(enemy, bn::move(chase_state));
            return;
        }

        // Set random direction if not already set
        if (!_direction_set)
        {
            static bn::random random;
            int angle = random.get() % 360;
            bn::fixed radians = angle * 3.14159 / 180;
            _target_dx = 0.35 * bn::sin(radians);
            _target_dy = 0.35 * bn::cos(radians);
            _direction_set = true;
        }

        // Apply movement target
        enemy._target_dx = _target_dx;
        enemy._target_dy = _target_dy;

        // Check if patrol time is up
        if (enemy._state_machine.get_state_timer() >= _patrol_duration)
        {
            // Transition back to idle state
            static bn::random random;
            int idle_duration = 20 + (random.get() % 40);
            bn::unique_ptr<IdleState> idle_state = bn::make_unique<IdleState>(idle_duration);
            enemy._state_machine.transition_to(enemy, bn::move(idle_state));
            return;
        }
    }

    void PatrolState::exit(Enemy &enemy)
    {
        // Stop movement when exiting patrol
        enemy._target_dx = 0;
        enemy._target_dy = 0;
    }

    // ChaseState implementation
    void ChaseState::enter(Enemy &enemy)
    {
        // No special setup needed for chase
    }

    void ChaseState::update(Enemy &enemy, bn::fixed_point player_pos, const Level &level, bool player_listening)
    {
        // Calculate distance to player
        bn::fixed dist_x = player_pos.x() - enemy.pos().x();
        bn::fixed dist_y = player_pos.y() - enemy.pos().y();
        bn::fixed dist_sq = dist_x * dist_x + dist_y * dist_y;

        // Different unfollow distances based on aggro state
        bn::fixed unfollow_dist_sq;
        if (enemy.type() == ENEMY_TYPE::SPEARGUARD && enemy._aggroed)
        {
            // Aggroed spearguards have much larger chase range (16 tiles instead of 8)
            unfollow_dist_sq = 128 * 128; // 16 tiles squared
        }
        else
        {
            // Normal chase range
            unfollow_dist_sq = 64 * 64; // 8 tiles squared
        }

        // Check if player is too far away or listening to NPCs
        if (dist_sq > unfollow_dist_sq || player_listening)
        {
            // Decide what to do based on enemy type
            if (enemy.type() == ENEMY_TYPE::SPEARGUARD)
            {
                // Spearguards return to their post
                bn::unique_ptr<ReturnToPostState> return_state = bn::make_unique<ReturnToPostState>();
                enemy._state_machine.transition_to(enemy, bn::move(return_state));
            }
            else
            {
                // Other enemies go idle
                static bn::random random;
                int idle_duration = 20 + (random.get() % 40);
                bn::unique_ptr<IdleState> idle_state = bn::make_unique<IdleState>(idle_duration);
                enemy._state_machine.transition_to(enemy, bn::move(idle_state));
            }
            return;
        }

        // Check if close enough to attack (spearguards only)
        if (enemy.type() == ENEMY_TYPE::SPEARGUARD && enemy._attack_timer <= 0)
        {
            // Spearguards should only attack when player is to their left or right
            // since the spear extends horizontally, not vertically
            bn::fixed abs_dist_x = bn::abs(dist_x);
            bn::fixed abs_dist_y = bn::abs(dist_y);

            // Only attack if the horizontal distance is significant relative to vertical distance
            // and the player is within reasonable attack range horizontally
            // Also ensure we're roughly Y-aligned (vertical distance is small)
            if (abs_dist_x <= ENEMY_ATTACK_DISTANCE && abs_dist_x >= abs_dist_y * 0.5 && abs_dist_y <= 16)
            {
                // Transition to attack state
                bn::unique_ptr<AttackState> attack_state = bn::make_unique<AttackState>();
                enemy._state_machine.transition_to(enemy, bn::move(attack_state));
                return;
            }
        }

        // Move toward player (spearguards prioritize getting to the same Y level for horizontal attacks)
        bn::fixed len = bn::sqrt(dist_sq);
        if (len > 0.1)
        {
            if (enemy.type() == ENEMY_TYPE::SPEARGUARD)
            {
                // Spearguards prioritize vertical alignment first, then horizontal positioning
                bn::fixed abs_dist_x = bn::abs(dist_x);
                bn::fixed abs_dist_y = bn::abs(dist_y);

                // If not Y-aligned, prioritize vertical movement
                if (abs_dist_y > 8) // Allow small Y tolerance
                {
                    enemy._target_dx = (dist_x / len) * _chase_speed * 0.3; // Slower horizontal movement
                    enemy._target_dy = (dist_y / len) * _chase_speed;       // Full vertical movement
                }
                else
                {
                    // Y-aligned, now move horizontally to attack position
                    enemy._target_dx = (dist_x / len) * _chase_speed;
                    enemy._target_dy = (dist_y / len) * _chase_speed * 0.3; // Maintain Y position
                }
            }
            else
            {
                // Other enemies move normally
                enemy._target_dx = (dist_x / len) * _chase_speed;
                enemy._target_dy = (dist_y / len) * _chase_speed;
            }
        }
        else
        {
            enemy._target_dx = 0;
            enemy._target_dy = 0;
        }
    }

    void ChaseState::exit(Enemy &enemy)
    {
        // No special cleanup needed
    }

    // AttackState implementation
    void AttackState::enter(Enemy &enemy)
    {
        // Start attack animation and stop movement
        enemy._attack_timer = _attack_duration;
        enemy._target_dx = 0;
        enemy._target_dy = 0;
    }

    void AttackState::update(Enemy &enemy, bn::fixed_point player_pos, const Level &level, bool player_listening)
    {
        // Keep enemy stationary during attack
        enemy._target_dx = 0;
        enemy._target_dy = 0;

        // Decrement attack timer
        if (enemy._attack_timer > 0)
        {
            enemy._attack_timer--;
        }

        // Check if attack is finished
        if (enemy._attack_timer <= 0)
        {
            // Check if player is still in range to continue chasing
            bn::fixed dist_x = player_pos.x() - enemy.pos().x();
            bn::fixed dist_y = player_pos.y() - enemy.pos().y();
            bn::fixed dist_sq = dist_x * dist_x + dist_y * dist_y;
            const bn::fixed follow_dist_sq = 48 * 48; // 6 tiles squared

            if (!player_listening && dist_sq <= follow_dist_sq)
            {
                // Continue chasing
                bn::unique_ptr<ChaseState> chase_state = bn::make_unique<ChaseState>();
                enemy._state_machine.transition_to(enemy, bn::move(chase_state));
            }
            else
            {
                // Player is too far or listening, decide what to do based on enemy type
                if (enemy.type() == ENEMY_TYPE::SPEARGUARD)
                {
                    // Return to post
                    bn::unique_ptr<ReturnToPostState> return_state = bn::make_unique<ReturnToPostState>();
                    enemy._state_machine.transition_to(enemy, bn::move(return_state));
                }
                else
                {
                    // Go idle
                    bn::unique_ptr<IdleState> idle_state = bn::make_unique<IdleState>();
                    enemy._state_machine.transition_to(enemy, bn::move(idle_state));
                }
            }
        }
    }

    void AttackState::exit(Enemy &enemy)
    {
        // Reset attack timer
        enemy._attack_timer = 0;
    }

    // ReturnToPostState implementation
    void ReturnToPostState::enter(Enemy &enemy)
    {
        // No special setup needed
    }

    void ReturnToPostState::update(Enemy &enemy, bn::fixed_point player_pos, const Level &level, bool player_listening)
    {
        // Calculate distance to original position
        bn::fixed dist_to_post_x = enemy._original_position.x() - enemy.pos().x();
        bn::fixed dist_to_post_y = enemy._original_position.y() - enemy.pos().y();
        bn::fixed dist_to_post_sq = dist_to_post_x * dist_to_post_x + dist_to_post_y * dist_to_post_y;

        // Check if close enough to original position
        if (dist_to_post_sq <= _threshold * _threshold)
        {
            // Snap to exact position and go idle
            enemy.set_position(enemy._original_position);

            // Clear aggro state when spearguard returns to post
            if (enemy.type() == ENEMY_TYPE::SPEARGUARD)
            {
                enemy._aggroed = false;
            }

            bn::unique_ptr<IdleState> idle_state = bn::make_unique<IdleState>();
            enemy._state_machine.transition_to(enemy, bn::move(idle_state));
            return;
        }

        // Move toward original position
        bn::fixed len = bn::sqrt(dist_to_post_sq);
        if (len > 0.1)
        {
            enemy._target_dx = (dist_to_post_x / len) * _return_speed;
            enemy._target_dy = (dist_to_post_y / len) * _return_speed;
        }
        else
        {
            enemy._target_dx = 0;
            enemy._target_dy = 0;
        }

        // Check if player comes back in range while returning
        bn::fixed dist_x = player_pos.x() - enemy.pos().x();
        bn::fixed dist_y = player_pos.y() - enemy.pos().y();
        bn::fixed dist_sq = dist_x * dist_x + dist_y * dist_y;
        const bn::fixed follow_dist_sq = 48 * 48; // 6 tiles squared

        if (!player_listening && dist_sq <= follow_dist_sq)
        {
            // Player is back in range, start chasing again
            bn::unique_ptr<ChaseState> chase_state = bn::make_unique<ChaseState>();
            enemy._state_machine.transition_to(enemy, bn::move(chase_state));
            return;
        }
    }

    void ReturnToPostState::exit(Enemy &enemy)
    {
        // No special cleanup needed
    }

    // StunnedState implementation
    void StunnedState::enter(Enemy &enemy)
    {
        // Stop movement while stunned
        enemy._target_dx = 0;
        enemy._target_dy = 0;
    }

    void StunnedState::update(Enemy &enemy, bn::fixed_point player_pos, const Level &level, bool player_listening)
    {
        // Keep enemy stationary while stunned
        enemy._target_dx = 0;
        enemy._target_dy = 0;

        // Check if stun duration is over
        if (enemy._state_machine.get_state_timer() >= _stun_duration)
        {
            // Special behavior for spearguards: always chase after being stunned (shot)
            if (enemy.type() == ENEMY_TYPE::SPEARGUARD)
            {
                // Spearguards always chase after being stunned, regardless of distance or listening state
                bn::unique_ptr<ChaseState> chase_state = bn::make_unique<ChaseState>();
                enemy._state_machine.transition_to(enemy, bn::move(chase_state));
                return;
            }

            // For other enemy types, decide what state to return to based on player proximity
            bn::fixed dist_x = player_pos.x() - enemy.pos().x();
            bn::fixed dist_y = player_pos.y() - enemy.pos().y();
            bn::fixed dist_sq = dist_x * dist_x + dist_y * dist_y;
            const bn::fixed follow_dist_sq = 48 * 48; // 6 tiles squared

            if (!player_listening && dist_sq <= follow_dist_sq)
            {
                // Player is close, start chasing
                bn::unique_ptr<ChaseState> chase_state = bn::make_unique<ChaseState>();
                enemy._state_machine.transition_to(enemy, bn::move(chase_state));
            }
            else
            {
                // Player is far, go idle
                bn::unique_ptr<IdleState> idle_state = bn::make_unique<IdleState>();
                enemy._state_machine.transition_to(enemy, bn::move(idle_state));
            }
        }
    }

    void StunnedState::exit(Enemy &enemy)
    {
        // Clear stunned flag
        enemy._stunned = false;
    }
}
