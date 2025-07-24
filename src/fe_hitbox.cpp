#include "fe_hitbox.h"

namespace fe
{
    Hitbox::Hitbox() : _pos(0, 0), _width(0), _height(0) {}

    Hitbox::Hitbox(bn::fixed x, bn::fixed y, bn::fixed width, bn::fixed height) : _pos(x, y), _width(width), _height(height)
    {
    }

    bn::fixed_point Hitbox::pos() const
    {
        return _pos;
    }

    bn::fixed Hitbox::x() const
    {
        return _pos.x();
    }

    bn::fixed Hitbox::y() const
    {
        return _pos.y();
    }

    bn::fixed Hitbox::width() const
    {
        return _width;
    }

    bn::fixed Hitbox::height() const
    {
        return _height;
    }

    void Hitbox::set_x(bn::fixed x)
    {
        _pos.set_x(x);
    }

    void Hitbox::set_y(bn::fixed y)
    {
        _pos.set_y(y);
    }

    void Hitbox::set_width(bn::fixed width)
    {
        _width = width;
    }

    void Hitbox::set_height(bn::fixed height)
    {
        _height = height;
    }

    void Hitbox::get_collision_points(bn::fixed_point pos, fe::directions direction, bn::fixed_point points[4]) const
    {
        // Edge offset to stay within bounds (one pixel inside the edge)
        constexpr bn::fixed EDGE_OFFSET = 1;
        
        // Calculate edge coordinates
        bn::fixed left = pos.x();
        bn::fixed right = pos.x() + _width - EDGE_OFFSET;
        bn::fixed top = pos.y();
        bn::fixed bottom = pos.y() + _height - EDGE_OFFSET;
        bn::fixed middle_x = pos.x() + _width / 2;
        bn::fixed quarter_x = pos.x() + _width / 4;
        bn::fixed middle_y = pos.y() + _height / 2;
        bn::fixed quarter_y = pos.y() + _height / 4;

        // Calculate points based on direction to check appropriate edges
        switch (direction)
        {
        case fe::directions::up:
            // Check top edge and corners when moving up
            points[0] = bn::fixed_point(left, top);     // Top-left
            points[1] = bn::fixed_point(right, top);    // Top-right
            points[2] = bn::fixed_point(middle_x, top); // Top-middle
            points[3] = bn::fixed_point(quarter_x, top);// Top-quarter
            break;
        case fe::directions::down:
            // Check bottom edge and corners when moving down
            points[0] = bn::fixed_point(left, bottom);     // Bottom-left
            points[1] = bn::fixed_point(right, bottom);    // Bottom-right
            points[2] = bn::fixed_point(middle_x, bottom); // Bottom-middle
            points[3] = bn::fixed_point(quarter_x, bottom);// Bottom-quarter
            break;
        case fe::directions::left:
            // Check left edge and corners when moving left
            points[0] = bn::fixed_point(left, top);       // Top-left
            points[1] = bn::fixed_point(left, bottom);    // Bottom-left
            points[2] = bn::fixed_point(left, middle_y);  // Middle-left
            points[3] = bn::fixed_point(left, quarter_y); // Quarter-left
            break;
        case fe::directions::right:
            // Check right edge and corners when moving right
            points[0] = bn::fixed_point(right, top);       // Top-right
            points[1] = bn::fixed_point(right, bottom);    // Bottom-right
            points[2] = bn::fixed_point(right, middle_y);  // Middle-right
            points[3] = bn::fixed_point(right, quarter_y); // Quarter-right
            break;
        default:
            // Default to all four corners
            points[0] = bn::fixed_point(left, top);    // Top-left
            points[1] = bn::fixed_point(right, top);   // Top-right
            points[2] = bn::fixed_point(left, bottom); // Bottom-left
            points[3] = bn::fixed_point(right, bottom);// Bottom-right
            break;
        }
    }
}