# 3D Engine Reference

The 3D engine is adapted from **varooom-3d** (`butano/games/varooom-3d/`). Stranded keeps the **butano submodule clean**: we do not modify varooom-3d files. Instead we use **project copies** of 3D sources in `src/viewer/` and **header overrides** in `include/` (see "Source Files" and "Stranded Extensions" below). This document covers the rendering pipeline, coordinate system, and isometric room viewer setup on the GBA (240x160).

---

## Coordinate System

```
Engine X  →  screen horizontal (left/right)
Engine Y  →  depth axis (camera looks toward -Y)
Engine Z  →  screen vertical (up/down, positive Z = lower on screen)
```

- Camera default position: `(0, 256, 0)`
- Camera looks toward **-Y** (decreasing Y = farther from camera)
- Screen center: pixel `(120, 80)`

### OBJ File Mapping

When importing from OBJ files, axes are swapped:

```
OBJ X  →  Engine X
OBJ Z  →  Engine Y  (depth)
OBJ Y  →  Engine Z
```

This Y/Z swap flips handedness, so OBJ vertex winding must be reversed to get CW winding for the rasterizer.

---

## Vertex Transformation Pipeline

Given a model vertex `(vx, vy, vz)`, the full pipeline to screen pixels is:

### Step 1: Rotation

The rotation matrix `R` is built from three Euler angles in the order **R = Rz(phi) * Ry(theta) * Rx(psi)**:

```
R = [ cφcθ     cφsθsψ - sφcψ     cφsθcψ + sφsψ ]
    [ sφcθ     sφsθsψ + cφcψ     sφsθcψ - cφsψ ]
    [ -sθ      cθsψ               cθcψ           ]
```

Applied as: `rotated = R * vertex`

### Step 2: Scale + Translate

```
world = scale * rotated + model_position
```

Order is always: rotate, then scale, then translate.

### Step 3: Camera-Space Transform

```
vrx = (world.x - camera.x) / 16
vrz = (world.z - camera.z) / 16
vcz = -(world.y - camera.y)          // depth (positive = in front of camera)
```

### Step 4: Project onto Camera Plane

The camera has a `phi` angle that sets orthonormal basis vectors `u` and `v` in the XZ plane:

```
u = (cos(camera_phi), 0, sin(camera_phi))    // screen horizontal
v = (sin(camera_phi), 0, -cos(camera_phi))   // screen vertical
```

Projection:
```
vcx =  dot(vrx, vrz, u) = vrx * u.x + vrz * u.z
vcy = -dot(vrx, vrz, v) = -(vrx * v.x + vrz * v.z)
```

### Step 5: Perspective Divide

```
scale = focal_length / vcz              // via lookup table for GBA speed
screen_x = (vcx * scale) >> 16 + 120   // center at 120
screen_y = (vcy * scale) >> 16 + 80    // center at 80
```

`focal_length_shift = 8` → focal length = 256.

---

## Euler Angle Reference

**All angles use 16-bit range: 0-65535 = 0°-360°.**

| Angle | Rotation | Axes Affected | What It Does |
|-------|----------|---------------|--------------|
| **phi** | Rz | X → Y | Rotates horizontal into **depth**. Changes which parts of the model are closer/farther from camera. **Creates perspective distortion.** |
| **theta** | Ry | X → Z | Rotates horizontal into vertical. Pure **screen-plane** rotation. **No depth effect.** |
| **psi** | Rx | Y → Z | Rotates depth into vertical. **Forward tilt** — reveals surfaces that face upward (like floors). |

### Key Insight

For an isometric view, you might expect to only need 2 rotations (tilt + screen rotation). But because `phi` (Rz) mixes into the depth axis, all **three** angles must work together to produce a symmetric diamond projection. Setting any one of them wrong breaks the symmetry.

---

## Isometric View Setup

### The Math

For the isometric diamond to be axis-aligned and symmetric on screen, the rotation matrix must satisfy:

1. `|R11| = |R12|` — equal horizontal screen contribution from model X and Y
2. `|R31| = |R32|` — equal vertical screen contribution

Solving these constraints yields the **exact isometric Euler angles**:

```
psi = arcsin(1/√3)                        = 35.264°
theta = -arctan(1/√3)                     = -30° (= 330°)
phi = arctan(cos(psi) / √(1 + sin²(psi))) = 35.264°
```

Note: `phi = psi = arctan(1/√2) ≈ 35.264°` — they are equal by mathematical necessity.

### Engine Values

Exact: `phi = 6425, theta = 60075, psi = 6425`

Nearest clean multiples of 256:

| Angle | Value | Calculation | Degrees |
|-------|-------|-------------|---------|
| phi | **6400** | 25 × 256 | 35.16° |
| theta | **59904** | 234 × 256 | 329° (= -31°) |
| psi | **6400** | 25 × 256 | 35.16° |

**Symmetry error: ~4%, sub-pixel at 240x160 resolution.**

### Position Values

| Parameter | Value | Purpose |
|-----------|-------|---------|
| room_x | 0 | Centered model |
| room_y | 96 | Depth position (6×16) |
| room_z | 16 | Vertical centering — walls extend ~36 units up after rotation, this shifts the visual center toward screen center |
| cam_y | 352 | Camera distance = 256 from room (352 - 96). Larger distance = less perspective distortion |

### Camera Phi

Set to **0** for isometric. The camera's `set_phi()` rotates the view plane in XZ — it could theoretically provide the screen rotation, but the model's three-angle system already handles everything. Mixing camera phi with model rotations complicates the math for no benefit.

---

## Perspective vs Isometric

The engine uses **perspective projection**, not orthographic. True sprite-based isometric games (like the reference screenshots) have no perspective foreshortening. To minimize the difference:

- **Increase camera distance** (cam_y - room_y). Larger distance = more orthographic-looking. Trade-off: the room appears smaller.
- **Current distance: 256 units** gives reasonable balance between room size and perspective distortion (~23% depth variation across the floor).

---

## Quick Reference: Common Angle Conversions

| Degrees | Engine Value | Notes |
|---------|-------------|-------|
| 22.5° | 4096 | 16 × 256 |
| 30° | 5461 | Not a clean 256-multiple |
| 33.75° | 6144 | 24 × 256 |
| 35.16° | 6400 | 25 × 256, closest to isometric 35.264° |
| 45° | 8192 | 32 × 256 |
| 90° | 16384 | 64 × 256 |
| 330° | 59904 | 234 × 256, equivalent to -31° |

**Formula**: `engine_value = degrees / 360 × 65536`

---

## Source Files

Stranded uses **project** paths so the butano submodule is unchanged. `Makefile` has `INCLUDES := include butano/common/include butano/games/varooom-3d/include`, so `include/` is resolved first.

| File | Contents |
|------|----------|
| `include/fr_model_3d.h` | **Project override.** Rotation matrix (`update()`), vertex transform (`rotate()`, `transform()`), `set_rotation_matrix()`, **depth_bias** for room shell sort order. |
| `include/fr_sprite_3d.h` | **Project override.** Sprite 3D transform; **horizontal_flip** for left-facing player. |
| `include/fr_sprite_3d_item.h` | **Project override.** **pixel_size**, **sprite_size**; supports 8/16/32/64 px sprites (not only 64). |
| `src/viewer/fr_models_3d.bn_iwram.cpp` | **Project copy.** Camera-space transform, perspective projection, depth sort (uses model `depth_bias()`), sprite_3d rendering (size, double-size, flip). |
| `src/viewer/fr_models_3d.cpp` | **Project copy.** Model/sprite list management, draw order. |
| `src/viewer/fr_camera_3d.cpp` | Camera phi → u/v basis vectors. |
| `include/fr_models_3d.h` | Models 3D manager (from common/varooom-3d). |
| `include/fr_camera_3d.h` | Camera class, default position (0, 256, 0) — from varooom-3d include. |

## Stranded Extensions

- **fr_model_3d:** `depth_bias()` / `set_depth_bias(int)` — room shell uses a large bias so it always draws behind furniture and player in depth sort.
- **fr_sprite_3d_item:** `pixel_size()`, `sprite_size()`; 8/16/32/64 px square sprites (eris 16×16 with double-size flag in rendering).
- **fr_sprite_3d:** `horizontal_flip()` / `set_horizontal_flip(bool)` — avoids negative scale assert when facing left.
- **Room viewer:** Scene in `src/core/room_viewer.cpp`; room/table/chair models in `include/models/str_model_3d_items_*.h`; furniture AABB hitboxes and slide-on-collide movement.
- **Submodule hygiene:** `butano/games/varooom-3d/include/` is left at upstream; all extensions live in `include/` and `src/viewer/`.

---

*3D engine reference. Updated 2026-02-21: project paths, Stranded extensions, submodule hygiene.*
