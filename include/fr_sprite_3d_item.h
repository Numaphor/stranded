/*
 * Copyright (c) 2020-2026 Gustavo Valiente gustavo.valiente@protonmail.com
 * zlib License, see LICENSE file.
 *
 * Stranded extension: support 8/16/32/64 pixel sprites (pixel_size, sprite_size).
 */

#ifndef FR_SPRITE_3D_ITEM_H
#define FR_SPRITE_3D_ITEM_H

#include "bn_sprite_item.h"
#include "bn_sprite_shape_size.h"
#include "bn_sprite_tiles_ptr.h"
#include "bn_sprite_palette_ptr.h"
#include "bn_sprite_affine_mat_ptr.h"

namespace fr
{

class sprite_3d_item
{

public:
    sprite_3d_item(const bn::sprite_item& item, int graphics_index) :
        _pixel_size(item.shape_size().width()),
        _tiles(item.tiles_item().create_tiles(graphics_index)),
        _palette(item.palette_item().create_palette()),
        _affine_mat(bn::sprite_affine_mat_ptr::create())
    {
        int w = item.shape_size().width();
        int h = item.shape_size().height();
        BN_ASSERT(w == h, "Sprite must be square: ", w, "x", h);
        BN_ASSERT(w == 8 || w == 16 || w == 32 || w == 64, "Invalid sprite size: ", w);

        _tiles_id = _tiles.id();
        _palette_id = _palette.id();
        _affine_mat_id = _affine_mat.id();
    }

    [[nodiscard]] const bn::sprite_tiles_ptr& tiles() const
    {
        return _tiles;
    }

    [[nodiscard]] bn::sprite_tiles_ptr& tiles()
    {
        return _tiles;
    }

    [[nodiscard]] int tiles_id() const
    {
        return _tiles_id;
    }

    [[nodiscard]] const bn::sprite_palette_ptr& palette() const
    {
        return _palette;
    }

    [[nodiscard]] bn::sprite_palette_ptr& palette()
    {
        return _palette;
    }

    [[nodiscard]] int palette_id() const
    {
        return _palette_id;
    }

    [[nodiscard]] const bn::sprite_affine_mat_ptr& affine_mat() const
    {
        return _affine_mat;
    }

    [[nodiscard]] bn::sprite_affine_mat_ptr& affine_mat()
    {
        return _affine_mat;
    }

    [[nodiscard]] int affine_mat_id() const
    {
        return _affine_mat_id;
    }

    [[nodiscard]] int pixel_size() const
    {
        return _pixel_size;
    }

    [[nodiscard]] bn::sprite_size sprite_size() const
    {
        switch(_pixel_size)
        {
        case 8:  return bn::sprite_size::SMALL;
        case 16: return bn::sprite_size::NORMAL;
        case 32: return bn::sprite_size::BIG;
        default: return bn::sprite_size::HUGE;
        }
    }

private:
    int _pixel_size;
    int _tiles_id;
    int _palette_id;
    int _affine_mat_id;
    bn::sprite_tiles_ptr _tiles;
    bn::sprite_palette_ptr _palette;
    bn::sprite_affine_mat_ptr _affine_mat;
};

}

#endif
