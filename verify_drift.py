"""
verify_drift.py -- Simulates the GBA projection pipeline with fixed-point
math to measure the exact player drift across corner rotations.

Run: python verify_drift.py
"""
import math

PI = math.pi

# ---------- bn::fixed simulation (Q12 -- 12 fractional bits) ----------

class Fixed:
    """Simulates Butano's bn::fixed (Q19.12 fixed-point)."""

    def __init__(self, val=0.0, *, raw=None):
        if raw is not None:
            self.data = int(raw)
        else:
            self.data = int(val * 4096)

    def to_double(self):
        return self.data / 4096.0

    def __add__(self, other):
        return Fixed(raw=self.data + other.data)

    def __sub__(self, other):
        return Fixed(raw=self.data - other.data)

    def __neg__(self):
        return Fixed(raw=-self.data)

    def __mul__(self, other):
        """safe_multiplication -- int64 path, matches bn::fixed::operator*"""
        return Fixed(raw=(self.data * other.data) >> 12)

    def unsafe_mul(self, other):
        """unsafe_multiplication -- int32 path (same result for values in [-1,1])"""
        return Fixed(raw=(self.data * other.data) >> 12)

    def __repr__(self):
        return f"{self.to_double():.5f}"


# ---------- Sin/Cos LUT simulation ----------

def angle_to_rad(angle):
    return angle * 2.0 * PI / 65536.0

def sim_sin(angle):
    angle = angle % 65536
    s = math.sin(angle_to_rad(angle))
    lut_val = int(round(s * 4096))
    lut_val = max(-32768, min(32767, lut_val))
    return Fixed(raw=lut_val)

def sim_cos(angle):
    return sim_sin(angle + 16384)


# ---------- div_lut simulation ----------

def sim_div_lut(index):
    if index < 2:
        return 1 << 24
    return (1 << 24) // index


# ---------- Karatsuba rotation (matches model_3d::rotate) ----------

class RotMatrix:
    def __init__(self, xx, xy, xz, yx, yy, yz, zx, zy, zz):
        self.xx = xx; self.xy = xy; self.xz = xz
        self.yx = yx; self.yy = yy; self.yz = yz
        self.zx = zx; self.zy = zy; self.zz = zz
        self.xx_xy = xx.unsafe_mul(xy)
        self.yx_yy = yx.unsafe_mul(yy)
        self.zx_zy = zx.unsafe_mul(zy)

    def rotate(self, vx, vy, vz, vxy):
        """Karatsuba trick matching model_3d::rotate()"""
        rx = (Fixed(raw=self.xx.data + vy.data) * Fixed(raw=self.xy.data + vx.data)
              + vz.unsafe_mul(self.xz) - self.xx_xy - vxy)
        ry = (Fixed(raw=self.yx.data + vy.data) * Fixed(raw=self.yy.data + vx.data)
              + vz.unsafe_mul(self.yz) - self.yx_yy - vxy)
        rz = (Fixed(raw=self.zx.data + vy.data) * Fixed(raw=self.zy.data + vx.data)
              + vz.unsafe_mul(self.zz) - self.zx_zy - vxy)
        return rx, ry, rz

    def transform(self, vx, vy, vz, pos_x, pos_y, pos_z):
        """Full transform: rotate + translate (matches model_3d::transform)"""
        vxy = vx * vy
        rx, ry, rz = self.rotate(vx, vy, vz, vxy)
        return rx + pos_x, ry + pos_y, rz + pos_z


# ---------- Projection (matches _floor_to_screen and _process_models) ----------

def project(world_x, world_y, world_z, cam_y):
    """Projects a world point to screen pixel offset from center."""
    vry_data = world_y.data - cam_y.data
    vcz = -vry_data
    if vcz <= 0:
        vcz = 1

    vrx = Fixed(raw=((world_x.data - 0) << 12) // (16 * 4096))
    vrz = Fixed(raw=((world_z.data - 0) << 12) // (16 * 4096))

    # camera phi=0: u=(1,0,0), v=(0,0,-1)
    vcx = vrx.data
    vcy = vrz.data

    focal_length_shift = 8
    lut_index = vcz >> 10
    if lut_index < 0:
        lut_index = 0
    scale = int((sim_div_lut(lut_index) << (focal_length_shift - 8)) >> 6)

    px = (vcx * scale) >> 16
    py = (vcy * scale) >> 16
    return px, py


# ---------- Corner matrices (matches compute_corner_matrices) ----------

def compute_corners():
    iso_phi, iso_theta, iso_psi = 6400, 59904, 6400

    sp = sim_sin(iso_phi); cp = sim_cos(iso_phi)
    st = sim_sin(iso_theta); ct = sim_cos(iso_theta)
    ss = sim_sin(iso_psi); cs = sim_cos(iso_psi)

    c0x = cp * ct
    c0y = cp * st * ss - sp * cs
    c0z = cp * st * cs + sp * ss
    c1x = sp * ct
    c1y = sp * st * ss + cp * cs
    c1z = sp * st * cs - cp * ss
    c2x = -st
    c2y = ct * ss
    c2z = ct * cs

    matrices = [
        RotMatrix(c0x, c0y, c0z, c1x, c1y, c1z, c2x, c2y, c2z),
        RotMatrix(c0y, -c0x, c0z, c1y, -c1x, c1z, c2y, -c2x, c2z),
        RotMatrix(-c0x, -c0y, c0z, -c1x, -c1y, c1z, -c2x, -c2y, c2z),
        RotMatrix(-c0y, c0x, c0z, -c1y, c1x, c1z, -c2y, c2x, c2z),
    ]
    return matrices


# ---------- Main ----------

def main():
    corners = compute_corners()

    room_pos = (Fixed(0), Fixed(96), Fixed(16))
    cam_y = Fixed(274)

    print("=== Corner Matrices ===")
    for c in range(4):
        m = corners[c]
        print(f"  Corner {c}: R00={m.xx} R01={m.xy} R10={m.yx} R11={m.yy}")
    print()

    # Player position
    player = ("PLAYER", 1.0, 6.0, 0.0)

    # Reference points
    refs = [
        # Floor-level (z=0) -- should track well
        ("floor_boundary(20,6,0)",     20.0,  6.0,  0.0),
        ("chair_leg_FL(6.64,12.64,0)", 6.64, 12.64, 0.0),
        ("chair_leg_FR(13.36,12.64)",  13.36, 12.64, 0.0),
        ("table_leg_FL(2.72,-4.48,0)", 2.72, -4.48, 0.0),
        ("nearby_floor(3,8,0)",        3.0,   8.0,  0.0),
        # Elevated (z<0) -- will show parallax
        ("chair_seat(10,16,-9.8)",     10.0, 16.0, -9.8),
        ("table_top(10,0,-14)",        10.0,  0.0, -14.0),
    ]

    # Compute player screen pos for each corner
    player_screens = []
    print("=== Player Screen Position ===")
    for c in range(4):
        wx, wy, wz = corners[c].transform(
            Fixed(player[1]), Fixed(player[2]), Fixed(player[3]),
            *room_pos)
        px, py = project(wx, wy, wz, cam_y)
        player_screens.append((px, py))
        print(f"  Corner {c}: screen=({px:4d}, {py:4d})  "
              f"world=({wx.to_double():8.3f}, {wy.to_double():8.3f}, {wz.to_double():8.3f})")
    print()

    # Analyze drift for each reference
    print("=== Drift Analysis ===")
    print("(delta = ref_screen - player_screen, drift = max - min across corners)\n")

    for name, rx, ry, rz in refs:
        print(f"--- {name} ---")
        deltas = []
        for c in range(4):
            wx, wy, wz = corners[c].transform(
                Fixed(rx), Fixed(ry), Fixed(rz), *room_pos)
            sx, sy = project(wx, wy, wz, cam_y)
            ppx, ppy = player_screens[c]
            dx, dy = sx - ppx, sy - ppy
            dist = math.sqrt(dx*dx + dy*dy)
            deltas.append((dx, dy))
            print(f"  C{c}: delta=({dx:4d},{dy:4d})  dist={dist:5.1f}")

        dists = [math.sqrt(d[0]**2 + d[1]**2) for d in deltas]
        dist_drift = max(dists) - min(dists)
        dx_vals = [d[0] for d in deltas]
        dy_vals = [d[1] for d in deltas]
        dx_range = max(dx_vals) - min(dx_vals)
        dy_range = max(dy_vals) - min(dy_vals)
        print(f"  >>> DISTANCE range: {min(dists):.1f} to {max(dists):.1f} px "
              f"(variation: {dist_drift:.1f} px)")
        print(f"      Delta range: dx={dx_range}px dy={dy_range}px "
              f"(expected: rotates ~90deg per corner)\n")


if __name__ == "__main__":
    main()
