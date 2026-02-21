#include <iostream>
#include <cmath>
#include <vector>
#include <iomanip>

// Simple fixed point simulation
struct fixed {
    double val;
    fixed(double v = 0) : val(v) {}
    operator double() const { return val; }
};

// Simulation of fr::sin/cos
// The engine uses a 65536 entry LUT. 65536 units = 360 degrees.
// So angle / 65536 * 2*PI radians.
double to_rad(int angle) {
    return (double)angle * 2.0 * 3.14159265358979323846 / 65536.0;
}

struct Matrix3x3 {
    double m[3][3];

    static Matrix3x3 identity() {
        Matrix3x3 mat;
        for(int i=0; i<3; ++i) for(int j=0; j<3; ++j) mat.m[i][j] = (i==j ? 1.0 : 0.0);
        return mat;
    }

    // Multiply: this * other
    Matrix3x3 operator*(const Matrix3x3& other) const {
        Matrix3x3 res;
        for(int i=0; i<3; ++i) {
            for(int j=0; j<3; ++j) {
                res.m[i][j] = 0;
                for(int k=0; k<3; ++k) {
                    res.m[i][j] += m[i][k] * other.m[k][j];
                }
            }
        }
        return res;
    }
};

// Compute matrix based on engine logic
Matrix3x3 compute_rotation(int phi, int theta, int psi) {
    double sp = std::sin(to_rad(phi)), cp = std::cos(to_rad(phi));
    double st = std::sin(to_rad(theta)), ct = std::cos(to_rad(theta));
    double ss = std::sin(to_rad(psi)), cs = std::cos(to_rad(psi));

    Matrix3x3 res;
    // Row 0
    res.m[0][0] = cp * ct;
    res.m[0][1] = cp * st * ss - sp * cs;
    res.m[0][2] = cp * st * cs + sp * ss; // Extrapolated from pattern (Rz*Ry*Rx?)

    // Row 1
    res.m[1][0] = sp * ct;
    res.m[1][1] = sp * st * ss + cp * cs;
    res.m[1][2] = sp * st * cs - cp * ss; // Extrapolated

    // Row 2
    res.m[2][0] = -st;
    res.m[2][1] = ct * ss;
    res.m[2][2] = ct * cs; // Extrapolated

    // Note: The code only used _r00, _r01, _r10, _r11, _r20, _r21.
    // _r02, _r12, _r22 were not explicitly calculated in the snippet provided, 
    // but usually needed for full 3D.
    // Let's verify row 2: _r20 = -st, _r21 = ct*ss.
    // This matches Ry(theta) * Rx(psi)?
    // Ry = [ct 0 st; 0 1 0; -st 0 ct]
    // Rx = [1 0 0; 0 cs -ss; 0 ss cs]
    // Ry*Rx = [ct, st*ss, st*cs; 0, cs, -ss; -st, ct*ss, ct*cs] -> This matches Row 2 perfectly!
    
    // Now check Rz(phi) * (Ry * Rx)
    // Rz = [cp -sp 0; sp cp 0; 0 0 1]
    // Rz * (Ry*Rx) = 
    // [cp*ct - sp*0,  cp*(st*ss) - sp*cs, ...] -> cp*ct, cp*st*ss - sp*cs. Matches Row 0!
    // [sp*ct + cp*0,  sp*(st*ss) + cp*cs, ...] -> sp*ct, sp*st*ss + cp*cs. Matches Row 1!
    
    // So the rotation is definitely R = Rz(phi) * Ry(theta) * Rx(psi).
    // Order: Roll(psi), then Pitch(theta), then Yaw(phi) applied to column vector?
    // v' = R * v.
    // So v is first rotated by Rx(psi), then Ry(theta), then Rz(phi).
    
    return res;
}

double diff_matrix(const Matrix3x3& a, const Matrix3x3& b) {
    double sum = 0;
    // We only care about columns 0 and 1 really (X and Y basis vectors), 
    // because Z is depth? No, Z is vertical.
    // The projection uses _r00.._r21.
    // Let's compare all 9 elements to be safe.
    for(int i=0; i<3; ++i) for(int j=0; j<3; ++j) sum += std::abs(a.m[i][j] - b.m[i][j]);
    return sum;
}

int main() {
    int iso_phi = 6400;
    int iso_theta = 59904;
    int iso_psi = 6400;

    Matrix3x3 M0 = compute_rotation(iso_phi, iso_theta, iso_psi);
    
    // Target Matrices for 90, 180, 270 degrees
    // We want to rotate the *Room* around its Z-axis (Vertical).
    // Since R = Rz(phi) * Ry(theta) * Rx(psi).
    // And the Room's local frame has Z as vertical.
    // If we apply Rz(90) * R_original? No.
    // The transformation is applied to vertex v: v' = R * v.
    // We want v_new = R * (RotZ(90) * v).
    // So R_new = R * RotZ(90).
    // Let's verify.
    // RotZ(90) = [0 -1 0; 1 0 0; 0 0 1].
    // So R_new column 0 = R column 1.
    // R_new column 1 = -R column 0.
    // R_new column 2 = R column 2.

    Matrix3x3 Targets[4];
    Targets[0] = M0;
    
    // Compute targets
    for(int k=1; k<4; ++k) {
        // Rotate 90 deg around Z relative to previous
        // New Col 0 = Old Col 1
        // New Col 1 = -Old Col 0
        // New Col 2 = Old Col 2
        Matrix3x3 prev = Targets[k-1];
        Matrix3x3 next;
        for(int i=0; i<3; ++i) {
            next.m[i][0] = -prev.m[i][1]; // Wait.
            // RotZ(90) * v: x' = -y, y' = x.
            // So if v=(1,0,0) [x-axis], RotZ(90)*v = (0,1,0) [y-axis].
            // So R_new * (1,0,0) should be R_old * (0,1,0).
            // Col 0 of R_new = Col 1 of R_old.
            
            // If v=(0,1,0) [y-axis], RotZ(90)*v = (-1,0,0) [-x-axis].
            // So R_new * (0,1,0) should be R_old * (-1,0,0) = -Col 0 of R_old.
            
            next.m[i][0] = prev.m[i][1];
            next.m[i][1] = -prev.m[i][0];
            next.m[i][2] = prev.m[i][2];
        }
        Targets[k] = next;
    }

    // Brute force solver
    // Range: 0 to 65535.
    // We can use gradient descent or coarse-to-fine search.
    // Since we know the solution is likely simple multiples or offsets.
    
    std::cout << "Solving...\n";

    for(int k=1; k<4; ++k) {
        double best_err = 1e9;
        int best_phi = 0, best_theta = 0, best_psi = 0;

        // Coarse search
        // Optimization: theta and psi might be related to original iso_theta/psi
        // Try fixing theta/psi to +/- iso values first?
        // Or search full space with step 1024?
        
        // Let's try to deduce analytically first.
        // R = Rz(phi) * Ry(theta) * Rx(psi)
        // We want R' = R * RotZ(90)
        // Rz(phi)*Ry(theta)*Rx(psi) * Rz(90) != Rz(phi+90)*...
        // Because rotations don't commute.
        // Unless Ry(theta)*Rx(psi) commutes with Rz(90)? Generally no.
        
        // So we really need to find (phi', theta', psi') such that
        // Rz(phi')*Ry(theta')*Rx(psi') = Target.
        
        // Decompose Target Matrix T into Euler angles (Z-Y-X).
        // T[2][0] = -sin(theta')
        // theta' = asin(-T[2][0])
        // T[2][1] = cos(theta') * sin(psi')
        // T[2][2] = cos(theta') * cos(psi')
        // psi' = atan2(T[2][1], T[2][2])
        // T[0][0] = cos(phi') * cos(theta')
        // T[1][0] = sin(phi') * cos(theta')
        // phi' = atan2(T[1][0], T[0][0])
        
        Matrix3x3 T = Targets[k];
        
        // Calculate Theta'
        // _r20 = -sin(theta)
        double st = -T.m[2][0];
        // clamp st to [-1, 1]
        if(st > 1.0) st = 1.0; if(st < -1.0) st = -1.0;
        double theta_rad = std::asin(st);
        
        // Calculate Psi'
        // _r21 = ct * ss, _r22 = ct * cs
        // tan(psi) = ss/cs = _r21 / _r22
        double psi_rad = std::atan2(T.m[2][1], T.m[2][2]);
        
        // Calculate Phi'
        // _r00 = cp * ct, _r10 = sp * ct
        // tan(phi) = sp/cp = _r10 / _r00
        double phi_rad = std::atan2(T.m[1][0], T.m[0][0]);
        
        // Convert to 0-65536
        auto to_int = [](double rad) {
            int val = (int)(rad * 65536.0 / (2.0 * 3.14159265358979323846));
            while(val < 0) val += 65536;
            return val % 65536;
        };
        
        best_phi = to_int(phi_rad);
        best_theta = to_int(theta_rad);
        best_psi = to_int(psi_rad);
        
        std::cout << "Corner " << k << ": " 
                  << "Phi=" << best_phi << " (" << phi_rad*180/3.14159 << " deg), "
                  << "Theta=" << best_theta << " (" << theta_rad*180/3.14159 << " deg), "
                  << "Psi=" << best_psi << " (" << psi_rad*180/3.14159 << " deg)\n";
                  
        // Verify
        Matrix3x3 M_calc = compute_rotation(best_phi, best_theta, best_psi);
        double err = diff_matrix(M_calc, T);
        std::cout << "  Error: " << err << "\n";
    }
    
    return 0;
}
