#ifndef FE_MULTIPLAYER_H
#define FE_MULTIPLAYER_H

#include "bn_optional.h"
#include "bn_fixed_point.h"
#include "bn_sprite_ptr.h"
#include "bn_camera_ptr.h"
#include "bn_vector.h"
#include "fe_player.h"

namespace fe
{
    // Maximum number of remote players (GBA link cable supports up to 4 total)
    constexpr int MAX_REMOTE_PLAYERS = 3;

    // Data packing for link transmission (16-bit data)
    // Encoding: 
    // - Bits 0-6: X position offset from center (-64 to +63) scaled
    // - Bits 7-13: Y position offset from center (-64 to +63) scaled
    // - Bits 14-15: Direction (0=UP, 1=DOWN, 2=LEFT, 3=RIGHT)
    
    // Position scale factor - divides actual world position for transmission.
    // With POSITION_SCALE=16 and 7-bit range (-64 to +63), the effective world
    // coordinate range is approximately -1024 to +1008, which covers most of
    // the playable map area (MAP_OFFSET is ~1280 from center).
    constexpr int POSITION_SCALE = 16;
    constexpr int POSITION_BITS = 7;
    constexpr int POSITION_MASK = 0x7F;  // 7 bits for position
    constexpr int DIRECTION_SHIFT = 14;
    constexpr int DIRECTION_MASK = 0x03;
    
    // Position encoding range constants
    constexpr int POSITION_MIN = -64;     // Minimum scaled position value
    constexpr int POSITION_MAX = 63;      // Maximum scaled position value  
    constexpr int POSITION_OFFSET = 64;   // Offset to make values unsigned (0-127)

    /**
     * @brief Represents a remote player's state received from link cable.
     */
    struct RemotePlayerState
    {
        int player_id = -1;
        bn::fixed_point position;
        PlayerMovement::Direction direction = PlayerMovement::Direction::DOWN;
        bool is_active = false;
        int frames_since_update = 0;  // Track staleness of data
    };

    /**
     * @brief Visual representation of a remote player.
     */
    class RemotePlayer
    {
    public:
        RemotePlayer();
        
        void initialize(int player_id, bn::camera_ptr camera);
        void update(const RemotePlayerState& state);
        void set_visible(bool visible);
        [[nodiscard]] bool is_initialized() const { return _initialized; }
        [[nodiscard]] int player_id() const { return _player_id; }
        [[nodiscard]] bn::fixed_point position() const { return _position; }
        [[nodiscard]] bn::sprite_ptr* get_sprite() { return _sprite.has_value() ? &_sprite.value() : nullptr; }
        
    private:
        int _player_id = -1;
        bn::fixed_point _position;
        bn::fixed_point _target_position;
        PlayerMovement::Direction _direction = PlayerMovement::Direction::DOWN;
        bn::optional<bn::sprite_ptr> _sprite;
        bn::optional<bn::sprite_animate_action<32>> _animation;
        bool _initialized = false;
        
        void update_animation();
        void interpolate_position();
    };

    /**
     * @brief Manages multiplayer communication and remote players.
     */
    class MultiplayerManager
    {
    public:
        MultiplayerManager();
        
        void initialize(bn::camera_ptr camera);
        void update(const Player& local_player);
        void shutdown();
        
        [[nodiscard]] bool is_connected() const { return _is_connected; }
        [[nodiscard]] int local_player_id() const { return _local_player_id; }
        [[nodiscard]] int player_count() const { return _player_count; }
        [[nodiscard]] const bn::vector<RemotePlayer, MAX_REMOTE_PLAYERS>& remote_players() const { return _remote_players; }
        [[nodiscard]] bn::vector<RemotePlayer, MAX_REMOTE_PLAYERS>& remote_players_mutable() { return _remote_players; }
        
    private:
        bn::vector<RemotePlayer, MAX_REMOTE_PLAYERS> _remote_players;
        bn::vector<RemotePlayerState, MAX_REMOTE_PLAYERS> _remote_states;
        bn::optional<bn::camera_ptr> _camera;
        int _local_player_id = 0;
        int _player_count = 1;
        bool _is_connected = false;
        bool _initialized = false;
        int _frames_since_send = 0;
        int _connection_timeout_frames = 0;
        
        static constexpr int SEND_INTERVAL = 2;  // Send data every 2 frames
        static constexpr int CONNECTION_TIMEOUT = 180;  // 3 seconds at 60fps
        
        void send_local_state(const Player& local_player);
        void receive_remote_states();
        int encode_player_data(bn::fixed_point pos, PlayerMovement::Direction dir) const;
        void decode_player_data(int data, bn::fixed_point& pos, PlayerMovement::Direction& dir) const;
    };
}

#endif // FE_MULTIPLAYER_H
