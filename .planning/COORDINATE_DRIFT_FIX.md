# Coordinate Drift Fix (RoomViewer Corner Switching)

## Root Cause

RoomViewer uses four “corner” orientations (corner index 0–3). The 3D room model orientation for each corner is defined by per-corner Euler angles, which produce a slightly different floor-to-world linear mapping per corner due to fixed-point trigonometry and Euler decomposition.

Before this change, corner switching treated the floor coordinates as if the four corner coordinate systems were related by an exact 90° rotation (swap/negate). That assumption is not strictly true for the actual per-corner Euler rotations being applied, so converting between corners by a simple 90° operation introduced a small positional offset (drift). Repeated corner switching compounded the error.

Separately, 16-bit angle wrapping in the 3D camera/model setters subtracted/added 0xFFFF instead of 0x10000, which introduces a 1-unit error when crossing the wrap boundary (accumulating over many rotations).

## Fix

### 1) World-Preserving Coordinate Transformation

When switching corners, the player floor position is converted using the actual floor-to-world mapping matrices of the old and new corners:

- Compute the offsetless world-space position `w = M_old * p_old`.
- Compute the new corner floor position with a least-squares pseudo-inverse:
  `p_new = (M_newᵀ M_new)⁻¹ M_newᵀ w`.

This ensures the same 3D world position is preserved across all corner transitions, with sub-centimeter accuracy.

Implementation: [room_viewer.cpp](file:///d:/repo/stranded/src/core/room_viewer.cpp)

### 2) Correct Angle Wrap Arithmetic

Angle wrapping now uses 0x10000 (65536) instead of 0xFFFF (65535) to match a full 16-bit circle.

Implementation:
- [fr_camera_3d.cpp](file:///d:/repo/stranded/src/viewer/fr_camera_3d.cpp)
- [fr_model_3d.h](file:///d:/repo/stranded/butano/games/varooom-3d/include/fr_model_3d.h)

## Validation Tests

RoomViewer runs a coordinate validation suite once on entry:

- Uses multiple fixed reference points (including fractional values).
- For every pair of corners (from→to), converts floor→world→floor and asserts:
  - World-space drift in x/y/z is ≤ 0.01 units.
  - Round-trip floor coordinate drift is ≤ 0.01 units.

Implementation: [room_viewer.cpp](file:///d:/repo/stranded/src/core/room_viewer.cpp)

