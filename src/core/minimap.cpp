#include "str_minimap.h"
#include "str_enemy.h"
#include "str_constants.h"

#include "bn_fixed_point.h"
#include "bn_sprite_builder.h"
#include "bn_blending.h"

#include "bn_sprite_items_minimap_room.h"
#include "bn_sprite_items_minimap_room_big.h"
#include "bn_sprite_items_minimap_bg.h"
#include "bn_sprite_items_minimap_arrow.h"
#include "bn_sprite_items_minimap_enemy.h"
#include "bn_sprite_items_minimap_door.h"

namespace str
{

    // =========================================================================
    // Minimap Implementation — Compact scrolling minimap
    // Player gem stays at panel center; rooms/doors scroll around it.
    // =========================================================================

    void Minimap::_configure_hud_sprite(bn::sprite_ptr& sprite, int z_order)
    {
        sprite.set_bg_priority(0);
        sprite.remove_camera();
        sprite.set_z_order(z_order);
        sprite.set_visible(true);
    }

    // Logical grid position for a room (relative to grid origin, NOT panel center)
    bn::fixed_point Minimap::_room_screen_pos(int room_id) const
    {
        int col = room_id % MINIMAP_ROOM_COLS;
        int row = room_id / MINIMAP_ROOM_COLS;
        bn::fixed x = MINIMAP_GRID_OFFSET_X + col * MINIMAP_GRID_CELL + (MINIMAP_ROOM_SIZE / 2);
        bn::fixed y = MINIMAP_GRID_OFFSET_Y + row * MINIMAP_GRID_CELL + (MINIMAP_ROOM_SIZE / 2);
        return bn::fixed_point(x, y);
    }

    int Minimap::_find_room(bn::fixed_point world_pos) const
    {
        int best_room = -1;
        bn::fixed best_score;

        for (int i = 0; i < MINIMAP_NUM_ROOMS; ++i)
        {
            bn::fixed dx = world_pos.x() - _room_center_x[i];
            bn::fixed dy = world_pos.y() - _room_center_y[i];
            if (bn::abs(dx) <= _room_half_x[i] && bn::abs(dy) <= _room_half_y[i])
            {
                bn::fixed score = (bn::abs(dx) / _room_half_x[i]) + (bn::abs(dy) / _room_half_y[i]);

                if(best_room < 0 || score < best_score || (score == best_score && i == _current_room))
                {
                    best_room = i;
                    best_score = score;
                }
            }
        }

        return best_room;
    }

    int Minimap::_find_room_room_viewer(bn::fixed_point world_pos) const
    {
        if(_current_room >= 0)
        {
            bn::fixed current_dx = bn::abs(world_pos.x() - _room_center_x[_current_room]);
            bn::fixed current_dy = bn::abs(world_pos.y() - _room_center_y[_current_room]);

            if(current_dx <= _room_half_x[_current_room] && current_dy <= _room_half_y[_current_room])
            {
                return _current_room;
            }
        }

        return _find_room(world_pos);
    }

    // Map world position to a logical minimap position (not screen position)
    // Returns coordinates in the same logical space as _room_screen_pos()
    bn::fixed_point Minimap::_world_to_minimap(bn::fixed_point world_pos, int room_id) const
    {
        bn::fixed_point room_logical = _room_screen_pos(room_id);

        // Normalized position within room (-1 to 1)
        bn::fixed nx = (world_pos.x() - _room_center_x[room_id]) / _room_half_x[room_id];
        bn::fixed ny = (world_pos.y() - _room_center_y[room_id]) / _room_half_y[room_id];

        // Scale to fit within the room sprite (leave 1px border)
        bn::fixed half_inner = _is_big_room[room_id] ? bn::fixed(6) : bn::fixed(5);
        // Negate X: isometric world +X = screen upper-right, minimap is top-down
        bn::fixed sx = room_logical.x() - nx * half_inner;
        bn::fixed sy = room_logical.y() + ny * half_inner;

        return bn::fixed_point(sx, sy);
    }

    bn::fixed_point Minimap::_world_to_minimap_room_viewer(bn::fixed_point world_pos, int room_id) const
    {
        // Room viewer movement and door triggers use local +/-55 as the playable bounds.
        constexpr bn::fixed room_viewer_playable_half_extent = bn::fixed(55);
        // Match the visible room art extents:
        // small room sprite ~= 12x12 visible area, big room ~= 14x14.
        bn::fixed room_viewer_half_inner = _is_big_room[room_id] ? bn::fixed(6) : bn::fixed(5);

        bn::fixed_point room_logical = _room_screen_pos(room_id);
        bn::fixed nx = (world_pos.x() - _room_center_x[room_id]) / room_viewer_playable_half_extent;
        bn::fixed ny = (world_pos.y() - _room_center_y[room_id]) / room_viewer_playable_half_extent;
        nx = bn::clamp(nx, bn::fixed(-1), bn::fixed(1));
        ny = bn::clamp(ny, bn::fixed(-1), bn::fixed(1));

        bn::fixed sx = room_logical.x() + nx * room_viewer_half_inner;
        bn::fixed sy = room_logical.y() + ny * room_viewer_half_inner;
        return bn::fixed_point(sx, sy);
    }

    void Minimap::_update_room_visuals()
    {
        // Show room state purely via visibility.
        // We avoid set_tiles entirely because sprite tile VRAM is fully consumed
        // by the 3D scene (0 free tiles). The gem marker shows the current room.
        //
        // UNVISITED = hidden (not yet discovered)
        // VISITED   = visible (discovered room)
        // CURRENT   = visible (gem marker is on it)
        for (int i = 0; i < MINIMAP_NUM_ROOMS; ++i)
        {
            _room_sprites[i].set_visible(_room_states[i] != RoomState::UNVISITED);
        }
    }

    void Minimap::_create_door_connectors()
    {
        for (int d = 0; d < DOOR_COUNT; ++d)
        {
            int room_a = _door_pairs[d][0];
            int room_b = _door_pairs[d][1];

            // Create at (0,0); positions are set dynamically in _scroll_sprites()
            bn::sprite_ptr door = bn::sprite_items::minimap_door.create_sprite(0, 0);
            _configure_hud_sprite(door, Z_ORDER_MINIMAP_DOOR);

            // If rooms are in the same row (horizontal connection), rotate 90 degrees
            int row_a = room_a / MINIMAP_ROOM_COLS;
            int row_b = room_b / MINIMAP_ROOM_COLS;
            if (row_a == row_b)
            {
                door.set_rotation_angle(90);
            }

            door.set_blending_enabled(true);
            _door_sprites.push_back(bn::move(door));
        }
    }

    // Reposition all room/door sprites based on scroll offset
    // Hide sprites that fall outside the 32x32 panel viewport
    void Minimap::_scroll_sprites(bn::fixed_point scroll_offset)
    {
        // Panel half-size: 32 (64x64 panel)
        // Clip rooms so they don't extend outside the panel edge
        // Room sprites are 16x16, door sprites are 8x8
        bn::fixed panel_half = 32;  // 64/2
        bn::fixed room_half = MINIMAP_ROOM_SIZE / 2;  // 8
        bn::fixed room_clip = panel_half - room_half;  // room center must be far enough inside
        bn::fixed door_clip = panel_half - 4;  // door center clipped with small margin

        // Reposition room sprites
        for (int i = 0; i < MINIMAP_NUM_ROOMS; ++i)
        {
            bn::fixed_point logical = _room_screen_pos(i);
            bn::fixed sx = logical.x() + scroll_offset.x();
            bn::fixed sy = logical.y() + scroll_offset.y();
            _room_sprites[i].set_position(sx, sy);

            // Only show if within clip bounds AND room has been visited
            bn::fixed dx = bn::abs(sx - _panel_center.x());
            bn::fixed dy = bn::abs(sy - _panel_center.y());
            bool in_bounds = (dx <= room_clip && dy <= room_clip);
            _room_sprites[i].set_visible(in_bounds && _room_states[i] != RoomState::UNVISITED);
        }

        // Reposition door sprites
        for (int d = 0; d < DOOR_COUNT && d < _door_sprites.size(); ++d)
        {
            int room_a = _door_pairs[d][0];
            int room_b = _door_pairs[d][1];

            bn::fixed_point pos_a = _room_screen_pos(room_a);
            bn::fixed_point pos_b = _room_screen_pos(room_b);

            bn::fixed mid_x = (pos_a.x() + pos_b.x()) / 2 + scroll_offset.x();
            bn::fixed mid_y = (pos_a.y() + pos_b.y()) / 2 + scroll_offset.y();
            _door_sprites[d].set_position(mid_x, mid_y);

            // Only show if both connected rooms are revealed AND within panel bounds
            bool room_a_revealed = (_room_states[room_a] != RoomState::UNVISITED);
            bool room_b_revealed = (_room_states[room_b] != RoomState::UNVISITED);
            bn::fixed dx = bn::abs(mid_x - _panel_center.x());
            bn::fixed dy = bn::abs(mid_y - _panel_center.y());
            _door_sprites[d].set_visible(room_a_revealed && room_b_revealed && dx <= door_clip && dy <= door_clip);
        }
    }

    void Minimap::_update_pulse()
    {
        _pulse_counter = (_pulse_counter + 1) % 60;

        int half = _pulse_counter;
        if (half > 30)
        {
            half = 60 - half;
        }
        bn::fixed alpha = bn::fixed(55 + half * 15 / 10) / 100;
        bn::blending::set_transparency_alpha(alpha);
    }

    Minimap::Minimap()
        : _bg_panel(bn::sprite_items::minimap_bg.create_sprite(MINIMAP_PANEL_X, MINIMAP_PANEL_Y)),
          _room_sprites{
              bn::sprite_items::minimap_room.create_sprite(0, 0, 0),      // Room 0: small
              bn::sprite_items::minimap_room_big.create_sprite(0, 0, 0),  // Room 1: BIG
              bn::sprite_items::minimap_room.create_sprite(0, 0, 0),      // Room 2: small
              bn::sprite_items::minimap_room.create_sprite(0, 0, 0),      // Room 3: small
              bn::sprite_items::minimap_room.create_sprite(0, 0, 0),      // Room 4: small
              bn::sprite_items::minimap_room_big.create_sprite(0, 0, 0)   // Room 5: BIG
          },
          _player_arrow(bn::sprite_items::minimap_arrow.create_sprite(MINIMAP_PANEL_X, MINIMAP_PANEL_Y, 0)),
          _current_room(-1),
          _panel_center(bn::fixed_point(MINIMAP_PANEL_X, MINIMAP_PANEL_Y)),
          _pulse_counter(0)
    {
        // Initialize room states
        for (int i = 0; i < MINIMAP_NUM_ROOMS; ++i)
        {
            _room_states[i] = RoomState::UNVISITED;
        }

        // Configure background panel
        _configure_hud_sprite(_bg_panel, Z_ORDER_MINIMAP_BG);
        _bg_panel.set_blending_enabled(true);
        bn::blending::set_transparency_alpha(0.7);

        // Initialize last-applied frames (all created with frame 0 = UNVISITED)
        for (int i = 0; i < MINIMAP_NUM_ROOMS; ++i)
        {
            _last_applied_frame[i] = 0;
        }
        // Configure room sprites (positions set in update via _scroll_sprites)
        for (int i = 0; i < MINIMAP_NUM_ROOMS; ++i)
        {
            _configure_hud_sprite(_room_sprites[i], Z_ORDER_MINIMAP_ROOM);
        }

        // Configure player arrow (stays at panel center)
        _configure_hud_sprite(_player_arrow, Z_ORDER_MINIMAP_PLAYER);

        // Create door connector sprites
        _create_door_connectors();

        // Initial scroll: center on room 0
        bn::fixed_point initial_logical = _room_screen_pos(0);
        bn::fixed_point scroll_offset(_panel_center.x() - initial_logical.x(),
                                       _panel_center.y() - initial_logical.y());
        _scroll_sprites(scroll_offset);
    }

    void Minimap::update(bn::fixed_point player_pos, int /*facing_direction*/, const bn::vector<Enemy, 16>& enemies)
    {
        int new_room = _find_room(player_pos);

        if (new_room >= 0)
        {
            // Only update visuals when room state actually changes
            bool room_changed = (_current_room != new_room);

            // Mark previous current room as visited
            if (_current_room >= 0 && room_changed)
            {
                _room_states[_current_room] = RoomState::VISITED;
            }

            if (room_changed)
            {
                _current_room = new_room;
                _room_states[_current_room] = RoomState::CURRENT;
                _update_room_visuals();
            }
            // Compute player's logical minimap position
            bn::fixed_point player_logical = _world_to_minimap(player_pos, _current_room);

            // Scroll offset: panel center - player logical position
            // This makes the player position appear at the panel center
            bn::fixed_point scroll_offset(_panel_center.x() - player_logical.x(),
                                           _panel_center.y() - player_logical.y());
            _scroll_sprites(scroll_offset);

            // Player gem stays at panel center
            _player_arrow.set_position(_panel_center);
            _player_arrow.set_visible(true);

            // Update enemy dots
            int visible_enemy_count = 0;
            for (int i = 0; i < enemies.size(); ++i)
            {
                bn::fixed_point ep = enemies[i].pos();
                int enemy_room = _find_room(ep);

                if (enemy_room < 0)
                {
                    continue;
                }

                if (visible_enemy_count >= _enemy_dots.size())
                {
                    auto s = bn::sprite_items::minimap_enemy.create_sprite(0, 0);
                    _configure_hud_sprite(s, Z_ORDER_MINIMAP_ENEMY);
                    s.set_blending_enabled(true);
                    _enemy_dots.push_back(EnemyDot(s, &enemies[i]));
                }

                // Enemy logical position + same scroll offset
                bn::fixed_point enemy_logical = _world_to_minimap(ep, enemy_room);
                bn::fixed_point enemy_screen(enemy_logical.x() + scroll_offset.x(),
                                              enemy_logical.y() + scroll_offset.y());
                _enemy_dots[visible_enemy_count].sprite.set_position(enemy_screen);

                // Clip enemy dots outside panel viewport
                bn::fixed edx = bn::abs(enemy_screen.x() - _panel_center.x());
                bn::fixed edy = bn::abs(enemy_screen.y() - _panel_center.y());
                bn::fixed enemy_clip = 16; // panel half
                _enemy_dots[visible_enemy_count].sprite.set_visible(edx <= enemy_clip && edy <= enemy_clip);
                _enemy_dots[visible_enemy_count].enemy = &enemies[i];
                ++visible_enemy_count;
            }

            // Hide excess enemy dots
            for (int i = visible_enemy_count; i < _enemy_dots.size(); ++i)
            {
                _enemy_dots[i].sprite.set_visible(false);
            }
            while (_enemy_dots.size() > enemies.size())
            {
                _enemy_dots.pop_back();
            }
        }
        else
        {
            _player_arrow.set_visible(false);
        }

        _update_pulse();
    }

    void Minimap::update(bn::fixed_point player_pos, int /*facing_direction*/)
    {
        int new_room = _find_room_room_viewer(player_pos);

        if (new_room >= 0)
        {
            bool room_changed = (_current_room != new_room);

            if (_current_room >= 0 && room_changed)
            {
                _room_states[_current_room] = RoomState::VISITED;
            }

            if (room_changed)
            {
                _current_room = new_room;
                _room_states[_current_room] = RoomState::CURRENT;
                _update_room_visuals();
            }
            // Compute player's logical minimap position
            bn::fixed_point player_logical = _world_to_minimap_room_viewer(player_pos, _current_room);

            // Scroll: rooms move, player gem stays at panel center
            bn::fixed_point scroll_offset(_panel_center.x() - player_logical.x(),
                                           _panel_center.y() - player_logical.y());
            _scroll_sprites(scroll_offset);

            _player_arrow.set_position(_panel_center);
            _player_arrow.set_visible(true);
        }
        else
        {
            _player_arrow.set_visible(false);
        }

        _update_pulse();
    }

    void Minimap::set_visible(bool visible)
    {
        _bg_panel.set_visible(visible);
        for (int i = 0; i < MINIMAP_NUM_ROOMS; ++i)
        {
            _room_sprites[i].set_visible(visible);
        }
        _player_arrow.set_visible(visible);
        for (auto& door : _door_sprites)
        {
            door.set_visible(visible);
        }
        for (auto& ed : _enemy_dots)
        {
            ed.sprite.set_visible(visible);
        }
    }

} // namespace str
