#include "fe_multiplayer.h"
#include "bn_link.h"
#include "bn_link_state.h"
#include "bn_sprite_builder.h"
#include "bn_sprite_items_hero_sword.h"
#include "bn_sprite_animate_actions.h"

namespace fe
{
    // ==================== RemotePlayer ====================

    RemotePlayer::RemotePlayer() :
        _player_id(-1),
        _position(0, 0),
        _target_position(0, 0),
        _direction(PlayerMovement::Direction::DOWN),
        _initialized(false)
    {
    }

    void RemotePlayer::initialize(int player_id, bn::camera_ptr camera)
    {
        _player_id = player_id;
        _position = bn::fixed_point(0, 0);
        _target_position = bn::fixed_point(0, 0);
        
        // Create sprite for remote player (using same hero sprite with different palette would be ideal)
        bn::sprite_builder builder(bn::sprite_items::hero_sword);
        builder.set_bg_priority(1);
        builder.set_position(_position);
        builder.set_camera(camera);
        
        // Set different visual appearance based on player ID
        // Player 0 is local, so remote players are 1, 2, 3
        // We could use palette swap but for now just use same sprite
        _sprite = builder.release_build();
        
        // Initialize walking animation (default down)
        _animation = bn::create_sprite_animate_action_forever(
            *_sprite, 8, bn::sprite_items::hero_sword.tiles_item(),
            0, 1, 2, 3, 4, 5, 6, 7
        );
        
        _initialized = true;
    }

    void RemotePlayer::update(const RemotePlayerState& state)
    {
        if (!_initialized || !state.is_active)
        {
            if (_sprite.has_value())
            {
                _sprite->set_visible(false);
            }
            return;
        }
        
        _target_position = state.position;
        _direction = state.direction;
        
        // Interpolate position for smooth movement
        interpolate_position();
        
        // Update sprite position and animation
        if (_sprite.has_value())
        {
            _sprite->set_visible(true);
            _sprite->set_position(_position);
            
            // Flip sprite based on direction
            bool facing_left = (_direction == PlayerMovement::Direction::LEFT);
            _sprite->set_horizontal_flip(facing_left);
        }
        
        update_animation();
    }

    void RemotePlayer::set_visible(bool visible)
    {
        if (_sprite.has_value())
        {
            _sprite->set_visible(visible);
        }
    }

    void RemotePlayer::interpolate_position()
    {
        // Smooth interpolation towards target position
        constexpr bn::fixed lerp_factor = 0.3;
        
        bn::fixed dx = _target_position.x() - _position.x();
        bn::fixed dy = _target_position.y() - _position.y();
        
        _position = bn::fixed_point(
            _position.x() + dx * lerp_factor,
            _position.y() + dy * lerp_factor
        );
    }

    void RemotePlayer::update_animation()
    {
        if (!_animation.has_value() || !_sprite.has_value())
        {
            return;
        }
        
        // Simple animation update - in a full implementation we'd change
        // animation based on direction and movement state
        _animation->update();
    }

    // ==================== MultiplayerManager ====================

    MultiplayerManager::MultiplayerManager() :
        _local_player_id(0),
        _player_count(1),
        _is_connected(false),
        _initialized(false),
        _frames_since_send(0),
        _connection_timeout_frames(0)
    {
    }

    void MultiplayerManager::initialize(bn::camera_ptr camera)
    {
        _camera = camera;
        _initialized = true;
        _is_connected = false;
        _local_player_id = 0;
        _player_count = 1;
        
        // Clear and initialize remote player states
        _remote_states.clear();
        _remote_players.clear();
        
        for (int i = 0; i < MAX_REMOTE_PLAYERS; ++i)
        {
            RemotePlayerState state;
            state.player_id = i + 1;  // Remote players have IDs 1, 2, 3
            state.is_active = false;
            _remote_states.push_back(state);
            
            RemotePlayer player;
            _remote_players.push_back(player);
        }
    }

    void MultiplayerManager::update(const Player& local_player)
    {
        if (!_initialized)
        {
            return;
        }
        
        // Send local player state periodically
        ++_frames_since_send;
        if (_frames_since_send >= SEND_INTERVAL)
        {
            send_local_state(local_player);
            _frames_since_send = 0;
        }
        
        // Receive remote player states
        receive_remote_states();
        
        // Update remote player visuals
        for (int i = 0; i < _remote_players.size() && i < _remote_states.size(); ++i)
        {
            // Initialize remote player sprite if needed
            if (!_remote_players[i].is_initialized() && _remote_states[i].is_active && _camera.has_value())
            {
                _remote_players[i].initialize(_remote_states[i].player_id, *_camera);
            }
            
            // Update remote player with latest state
            _remote_players[i].update(_remote_states[i]);
            
            // Track staleness
            if (_remote_states[i].is_active)
            {
                _remote_states[i].frames_since_update++;
                
                // Mark as inactive if no updates received for too long
                if (_remote_states[i].frames_since_update > CONNECTION_TIMEOUT)
                {
                    _remote_states[i].is_active = false;
                }
            }
        }
        
        // Update connection status
        if (_is_connected)
        {
            _connection_timeout_frames++;
            if (_connection_timeout_frames > CONNECTION_TIMEOUT)
            {
                _is_connected = false;
                _player_count = 1;
            }
        }
    }

    void MultiplayerManager::shutdown()
    {
        // Deactivate link communication
        bn::link::deactivate();
        
        _is_connected = false;
        _initialized = false;
        
        // Clear remote players
        _remote_players.clear();
        _remote_states.clear();
    }

    void MultiplayerManager::send_local_state(const Player& local_player)
    {
        int data = encode_player_data(local_player.pos(), local_player.facing_direction());
        bn::link::send(data);
    }

    void MultiplayerManager::receive_remote_states()
    {
        // Try to receive link state multiple times per frame
        constexpr int max_retries = 5;
        
        for (int retry = 0; retry < max_retries; ++retry)
        {
            bn::optional<bn::link_state> link_state = bn::link::receive();
            
            if (link_state.has_value())
            {
                // Successfully received data - update connection status
                _is_connected = true;
                _connection_timeout_frames = 0;
                _local_player_id = link_state->current_player_id();
                _player_count = link_state->player_count();
                
                // Process data from each remote player
                const bn::ivector<bn::link_player>& other_players = link_state->other_players();
                
                for (const bn::link_player& player : other_players)
                {
                    int remote_id = player.id();
                    
                    // Find the corresponding remote state slot
                    // Remote IDs are 0-3, but we skip our own ID
                    int state_index = remote_id;
                    if (remote_id > _local_player_id)
                    {
                        state_index--;
                    }
                    
                    if (state_index >= 0 && state_index < _remote_states.size())
                    {
                        bn::fixed_point pos;
                        PlayerMovement::Direction dir;
                        decode_player_data(player.data(), pos, dir);
                        
                        _remote_states[state_index].player_id = remote_id;
                        _remote_states[state_index].position = pos;
                        _remote_states[state_index].direction = dir;
                        _remote_states[state_index].is_active = true;
                        _remote_states[state_index].frames_since_update = 0;
                    }
                }
            }
            else
            {
                // No more data available
                break;
            }
        }
    }

    int MultiplayerManager::encode_player_data(bn::fixed_point pos, PlayerMovement::Direction dir) const
    {
        // Scale and clamp position to fit in 7 bits each (-64 to +63)
        int x = bn::clamp(pos.x().integer() / POSITION_SCALE, -64, 63) + 64;  // 0-127
        int y = bn::clamp(pos.y().integer() / POSITION_SCALE, -64, 63) + 64;  // 0-127
        
        // Encode direction as 2 bits
        int dir_bits = 0;
        switch (dir)
        {
            case PlayerMovement::Direction::UP:    dir_bits = 0; break;
            case PlayerMovement::Direction::DOWN:  dir_bits = 1; break;
            case PlayerMovement::Direction::LEFT:  dir_bits = 2; break;
            case PlayerMovement::Direction::RIGHT: dir_bits = 3; break;
        }
        
        // Pack into 16-bit value: DDYYYYYYYXXXXXXX
        int data = (x & POSITION_MASK) |
                   ((y & POSITION_MASK) << POSITION_BITS) |
                   ((dir_bits & DIRECTION_MASK) << DIRECTION_SHIFT);
        
        return data;
    }

    void MultiplayerManager::decode_player_data(int data, bn::fixed_point& pos, PlayerMovement::Direction& dir) const
    {
        // Extract position (7 bits each, offset by 64)
        int x = (data & POSITION_MASK) - 64;
        int y = ((data >> POSITION_BITS) & POSITION_MASK) - 64;
        
        // Scale back to world coordinates
        pos = bn::fixed_point(x * POSITION_SCALE, y * POSITION_SCALE);
        
        // Extract direction (2 bits)
        int dir_bits = (data >> DIRECTION_SHIFT) & DIRECTION_MASK;
        switch (dir_bits)
        {
            case 0: dir = PlayerMovement::Direction::UP;    break;
            case 1: dir = PlayerMovement::Direction::DOWN;  break;
            case 2: dir = PlayerMovement::Direction::LEFT;  break;
            case 3: dir = PlayerMovement::Direction::RIGHT; break;
            default: dir = PlayerMovement::Direction::DOWN; break;
        }
    }
}
