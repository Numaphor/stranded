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
        // Calculate points based on direction to check appropriate edges
        switch (direction)
        {
        case fe::directions::up:
            // Check top edge and corners when moving up
            points[0] = bn::fixed_point(pos.x(), pos.y());              // Top-left
            points[1] = bn::fixed_point(pos.x() + _width - 1, pos.y()); // Top-right
            points[2] = bn::fixed_point(pos.x() + _width / 2, pos.y()); // Top-middle
            points[3] = bn::fixed_point(pos.x() + _width / 4, pos.y()); // Top-quarter
            break;
        case fe::directions::down:
            // Check bottom edge and corners when moving down
            points[0] = bn::fixed_point(pos.x(), pos.y() + _height - 1);              // Bottom-left
            points[1] = bn::fixed_point(pos.x() + _width - 1, pos.y() + _height - 1); // Bottom-right
            points[2] = bn::fixed_point(pos.x() + _width / 2, pos.y() + _height - 1); // Bottom-middle
            points[3] = bn::fixed_point(pos.x() + _width / 4, pos.y() + _height - 1); // Bottom-quarter
            break;
        case fe::directions::left:
            // Check left edge and corners when moving left
            points[0] = bn::fixed_point(pos.x(), pos.y());               // Top-left
            points[1] = bn::fixed_point(pos.x(), pos.y() + _height - 1); // Bottom-left
            points[2] = bn::fixed_point(pos.x(), pos.y() + _height / 2); // Middle-left
            points[3] = bn::fixed_point(pos.x(), pos.y() + _height / 4); // Quarter-left
            break;
        case fe::directions::right:
            // Check right edge and corners when moving right
            points[0] = bn::fixed_point(pos.x() + _width - 1, pos.y());               // Top-right
            points[1] = bn::fixed_point(pos.x() + _width - 1, pos.y() + _height - 1); // Bottom-right
            points[2] = bn::fixed_point(pos.x() + _width - 1, pos.y() + _height / 2); // Middle-right
            points[3] = bn::fixed_point(pos.x() + _width - 1, pos.y() + _height / 4); // Quarter-right
            break;
        default:
            // Default to all four corners
            points[0] = bn::fixed_point(pos.x(), pos.y());                            // Top-left
            points[1] = bn::fixed_point(pos.x() + _width - 1, pos.y());               // Top-right
            points[2] = bn::fixed_point(pos.x(), pos.y() + _height - 1);              // Bottom-left
            points[3] = bn::fixed_point(pos.x() + _width - 1, pos.y() + _height - 1); // Bottom-right
            break;
        }
    }
}