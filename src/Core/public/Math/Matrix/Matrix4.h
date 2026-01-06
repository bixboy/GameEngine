#pragma once
#include <cmath>
#include <cstring>
#include "Math/Vector3.h"
#include "Math/Rotator.h"

namespace BixEngine::Math
{
    struct Matrix4
    {
        float m[4][4] = {};

        constexpr Matrix4()
            : m{
                {1.0f, 0.0f, 0.0f, 0.0f},
                {0.0f, 1.0f, 0.0f, 0.0f},
                {0.0f, 0.0f, 1.0f, 0.0f},
                {0.0f, 0.0f, 0.0f, 1.0f}
            }
        {
        }

        constexpr Matrix4(float diagonal)
            : m{
                {diagonal, 0.0f, 0.0f, 0.0f},
                {0.0f, diagonal, 0.0f, 0.0f},
                {0.0f, 0.0f, diagonal, 0.0f},
                {0.0f, 0.0f, 0.0f, diagonal}
            }
        {
        }

        static constexpr Matrix4 Identity()
        {
            return Matrix4();
        }

        void SetIdentity()
        {
            // Reset to identity
            for(int i=0; i<4; ++i)
                for(int j=0; j<4; ++j)
                    m[i][j] = (i == j) ? 1.0f : 0.0f;
        }

        [[nodiscard]] Rotator GetRotation() const
        {
            // Simple extraction (assuming scale is 1 or uniform)
            // This is a basic implementation to satisfy the compilation
            Vector3 forward = { m[0][2], m[1][2], m[2][2] };
            Vector3 up = { m[0][1], m[1][1], m[2][1] };
            
            // Normalize to handle scaling
            forward = forward.Normalized();
            up = up.Normalized();
            
            Vector3 right = up.Cross(forward);
            
            // Convert basis to Euler (simplistic)
            // In a real engine, use a robust Decompose or Quaternion conversion
            // This creates a Rotator from Forward vector, which matches 'LookAt' usage
            // But we might need full 3-axis rotation. 
            // For now, using what Rotator provides or minimal math.
            
            float pitch = std::asin(-forward.y) * Rotator::kRadiansToDegrees;
            float yaw = std::atan2(forward.x, forward.z) * Rotator::kRadiansToDegrees;
            float roll = 0.0f; // Hard to deduce from just forward/up without more math
            
            return Rotator(pitch, yaw, roll);
        }
        
        [[nodiscard]] Matrix4 operator*(const Matrix4& other) const
        {
            Matrix4 res;
            for (int r = 0; r < 4; ++r)
            {
                for (int c = 0; c < 4; ++c)
                {
                    res.m[r][c] = 
                        m[r][0] * other.m[0][c] +
                        m[r][1] * other.m[1][c] +
                        m[r][2] * other.m[2][c] +
                        m[r][3] * other.m[3][c];
                }
            }
            
            return res;
        }

        [[nodiscard]] Vector3 MultiplyPoint(const Vector3& v) const
        {
            float x = m[0][0]*v.x + m[0][1]*v.y + m[0][2]*v.z + m[0][3];
            float y = m[1][0]*v.x + m[1][1]*v.y + m[1][2]*v.z + m[1][3];
            float z = m[2][0]*v.x + m[2][1]*v.y + m[2][2]*v.z + m[2][3];
            
            return {x, y, z};
        }

        [[nodiscard]] Vector3 MultiplyDirection(const Vector3& v) const
        {
            float x = m[0][0]*v.x + m[0][1]*v.y + m[0][2]*v.z;
            float y = m[1][0]*v.x + m[1][1]*v.y + m[1][2]*v.z;
            float z = m[2][0]*v.x + m[2][1]*v.y + m[2][2]*v.z;
            
            return {x, y, z};
        }

        // --- Builders ---

        [[nodiscard]] float Determinant() const
        {
            return 
                m[0][3] * m[1][2] * m[2][1] * m[3][0] - m[0][2] * m[1][3] * m[2][1] * m[3][0] -
                m[0][3] * m[1][1] * m[2][2] * m[3][0] + m[0][1] * m[1][3] * m[2][2] * m[3][0] +
                m[0][2] * m[1][1] * m[2][3] * m[3][0] - m[0][1] * m[1][2] * m[2][3] * m[3][0] -
                m[0][3] * m[1][2] * m[2][0] * m[3][1] + m[0][2] * m[1][3] * m[2][0] * m[3][1] +
                m[0][3] * m[1][0] * m[2][2] * m[3][1] - m[0][0] * m[1][3] * m[2][2] * m[3][1] -
                m[0][2] * m[1][0] * m[2][3] * m[3][1] + m[0][0] * m[1][2] * m[2][3] * m[3][1] +
                m[0][3] * m[1][1] * m[2][0] * m[3][2] - m[0][1] * m[1][3] * m[2][0] * m[3][2] -
                m[0][3] * m[1][0] * m[2][1] * m[3][2] + m[0][0] * m[1][3] * m[2][1] * m[3][2] +
                m[0][1] * m[1][0] * m[2][3] * m[3][2] - m[0][0] * m[1][1] * m[2][3] * m[3][2] -
                m[0][2] * m[1][1] * m[2][0] * m[3][3] + m[0][1] * m[1][2] * m[2][0] * m[3][3] +
                m[0][2] * m[1][0] * m[2][1] * m[3][3] - m[0][0] * m[1][2] * m[2][1] * m[3][3] -
                m[0][1] * m[1][0] * m[2][2] * m[3][3] + m[0][0] * m[1][1] * m[2][2] * m[3][3];
        }

        [[nodiscard]] Matrix4 Inverse() const
        {
            float det = Determinant();
            
            if (std::abs(det) < 1e-6f)
                return Identity();

            Matrix4 inv;
            float invDet = 1.0f / det;

            // Ligne 0
            inv.m[0][0] = (m[1][1] * m[2][2] * m[3][3] + m[1][2] * m[2][3] * m[3][1] + m[1][3] * m[2][1] * m[3][2] - m[1][1] * m[2][3] * m[3][2] - m[1][2] * m[2][1] * m[3][3] - m[1][3] * m[2][2] * m[3][1]) * invDet;
            inv.m[0][1] = (m[0][1] * m[2][3] * m[3][2] + m[0][2] * m[2][1] * m[3][3] + m[0][3] * m[2][2] * m[3][1] - m[0][1] * m[2][2] * m[3][3] - m[0][2] * m[2][3] * m[3][1] - m[0][3] * m[2][1] * m[3][2]) * invDet;
            inv.m[0][2] = (m[0][1] * m[1][2] * m[3][3] + m[0][2] * m[1][3] * m[3][1] + m[0][3] * m[1][1] * m[3][2] - m[0][1] * m[1][3] * m[3][2] - m[0][2] * m[1][1] * m[3][3] - m[0][3] * m[1][2] * m[3][1]) * invDet;
            inv.m[0][3] = (m[0][1] * m[1][3] * m[2][2] + m[0][2] * m[1][1] * m[2][3] + m[0][3] * m[1][2] * m[2][1] - m[0][1] * m[1][2] * m[2][3] - m[0][2] * m[1][3] * m[2][1] - m[0][3] * m[1][1] * m[2][2]) * invDet;

            // Ligne 1
            inv.m[1][0] = (m[1][0] * m[2][3] * m[3][2] + m[1][2] * m[2][0] * m[3][3] + m[1][3] * m[2][2] * m[3][0] - m[1][0] * m[2][2] * m[3][3] - m[1][2] * m[2][3] * m[3][0] - m[1][3] * m[2][0] * m[3][2]) * invDet;
            inv.m[1][1] = (m[0][0] * m[2][2] * m[3][3] + m[0][2] * m[2][3] * m[3][0] + m[0][3] * m[2][0] * m[3][2] - m[0][0] * m[2][3] * m[3][2] - m[0][2] * m[2][0] * m[3][3] - m[0][3] * m[2][2] * m[3][0]) * invDet;
            inv.m[1][2] = (m[0][0] * m[1][3] * m[3][2] + m[0][2] * m[1][0] * m[3][3] + m[0][3] * m[1][2] * m[3][0] - m[0][0] * m[1][2] * m[3][3] - m[0][2] * m[1][3] * m[3][0] - m[0][3] * m[1][0] * m[3][2]) * invDet;
            inv.m[1][3] = (m[0][0] * m[1][2] * m[2][3] + m[0][2] * m[1][3] * m[2][0] + m[0][3] * m[1][0] * m[2][2] - m[0][0] * m[1][3] * m[2][2] - m[0][2] * m[1][0] * m[2][3] - m[0][3] * m[1][2] * m[2][0]) * invDet;

            // Ligne 2
            inv.m[2][0] = (m[1][0] * m[2][1] * m[3][3] + m[1][1] * m[2][3] * m[3][0] + m[1][3] * m[2][0] * m[3][1] - m[1][0] * m[2][3] * m[3][1] - m[1][1] * m[2][0] * m[3][3] - m[1][3] * m[2][1] * m[3][0]) * invDet;
            inv.m[2][1] = (m[0][0] * m[2][3] * m[3][1] + m[0][1] * m[2][0] * m[3][3] + m[0][3] * m[2][1] * m[3][0] - m[0][0] * m[2][1] * m[3][3] - m[0][1] * m[2][3] * m[3][0] - m[0][3] * m[2][0] * m[3][1]) * invDet;
            inv.m[2][2] = (m[0][0] * m[1][1] * m[3][3] + m[0][1] * m[1][3] * m[3][0] + m[0][3] * m[1][0] * m[3][1] - m[0][0] * m[1][3] * m[3][1] - m[0][1] * m[1][0] * m[3][3] - m[0][3] * m[1][1] * m[3][0]) * invDet;
            inv.m[2][3] = (m[0][0] * m[1][3] * m[2][1] + m[0][1] * m[1][0] * m[2][3] + m[0][3] * m[1][1] * m[2][0] - m[0][0] * m[1][1] * m[2][3] - m[0][1] * m[1][3] * m[2][0] - m[0][3] * m[1][0] * m[2][1]) * invDet;

            // Ligne 3
            inv.m[3][0] = (m[1][0] * m[2][2] * m[3][1] + m[1][1] * m[2][0] * m[3][2] + m[1][2] * m[2][1] * m[3][0] - m[1][0] * m[2][1] * m[3][2] - m[1][1] * m[2][2] * m[3][0] - m[1][2] * m[2][0] * m[3][1]) * invDet;
            inv.m[3][1] = (m[0][0] * m[2][1] * m[3][2] + m[0][1] * m[2][2] * m[3][0] + m[0][2] * m[2][0] * m[3][1] - m[0][0] * m[2][2] * m[3][1] - m[0][1] * m[2][0] * m[3][2] - m[0][2] * m[2][1] * m[3][0]) * invDet;
            inv.m[3][2] = (m[0][0] * m[1][2] * m[3][1] + m[0][1] * m[1][0] * m[3][2] + m[0][2] * m[1][1] * m[3][0] - m[0][0] * m[1][1] * m[3][2] - m[0][1] * m[1][2] * m[3][0] - m[0][2] * m[1][0] * m[3][1]) * invDet;
            inv.m[3][3] = (m[0][0] * m[1][1] * m[2][2] + m[0][1] * m[1][2] * m[2][0] + m[0][2] * m[1][0] * m[2][1] - m[0][0] * m[1][2] * m[2][1] - m[0][1] * m[1][0] * m[2][2] - m[0][2] * m[1][1] * m[2][0]) * invDet;

            return inv;
        }
        
        static Matrix4 TRS(const Vector3& pos, const Rotator& rot, const Vector3& scale)
        {
            Matrix4 mat;
            
            float cp = std::cos(rot.PitchRad());
            float sp = std::sin(rot.PitchRad());
            float cy = std::cos(rot.YawRad());
            float sy = std::sin(rot.YawRad());
            float cr = std::cos(rot.RollRad());
            float sr = std::sin(rot.RollRad());
            
            // Axe X (Right)
            mat.m[0][0] = (cy * cr + sy * sp * sr) * scale.x;
            mat.m[1][0] = (cr * sp * sy - cy * sr) * scale.x;
            mat.m[2][0] = (cp * sy) * scale.x;

            // Axe Y (Up)
            mat.m[0][1] = (cp * sr) * scale.y;
            mat.m[1][1] = (cp * cr) * scale.y;
            mat.m[2][1] = (-sp) * scale.y;

            // Axe Z (Forward)
            mat.m[0][2] = (cy * sp * sr - sy * cr) * scale.z;
            mat.m[1][2] = (sy * sr + cy * cr * sp) * scale.z;
            mat.m[2][2] = (cp * cy) * scale.z;

            // Translation
            mat.m[0][3] = pos.x;
            mat.m[1][3] = pos.y;
            mat.m[2][3] = pos.z;
            mat.m[3][3] = 1.0f;

            return mat;
        }

        static Matrix4 LookAt(const Vector3& eye, const Vector3& target, const Vector3& up)
        {
            Vector3 f = (target - eye).Normalized();
            Vector3 r = f.Cross(up).Normalized();
            Vector3 u = r.Cross(f);

            Matrix4 res;
            res.m[0][0] = r.x; res.m[0][1] = r.y; res.m[0][2] = r.z; res.m[0][3] = -r.Dot(eye);
            res.m[1][0] = u.x; res.m[1][1] = u.y; res.m[1][2] = u.z; res.m[1][3] = -u.Dot(eye);
            res.m[2][0] = -f.x; res.m[2][1] = -f.y; res.m[2][2] = -f.z; res.m[2][3] = f.Dot(eye);
            res.m[3][0] = 0;   res.m[3][1] = 0;   res.m[3][2] = 0;   res.m[3][3] = 1;
            return res;
        }
        
        [[nodiscard]] const float* Data() const
        {
            return &m[0][0];
        }
        
        [[nodiscard]] Vector3 GetTranslation() const
        {
            return { m[0][3], m[1][3], m[2][3] };
        }
    };
}