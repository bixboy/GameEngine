#pragma once
#include <cmath>
#include "Math/Vector3.h"


namespace BixEngine::Math
{
    struct Matrix3
    {
        float m[3][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};

        constexpr Matrix3() = default;

        constexpr Matrix3(
            float m00, float m01, float m02,
            float m10, float m11, float m12,
            float m20, float m21, float m22)
        {
            m[0][0] = m00;
            m[0][1] = m01;
            m[0][2] = m02;
            m[1][0] = m10;
            m[1][1] = m11;
            m[1][2] = m12;
            m[2][0] = m20;
            m[2][1] = m21;
            m[2][2] = m22;
        }

        
        
        
        [[nodiscard]] static constexpr Matrix3 Identity() noexcept
        {
            return Matrix3(
                1, 0, 0,
                0, 1, 0,
                0, 0, 1
            );
        }

        
        
        
        [[nodiscard]] constexpr Matrix3 operator*(const Matrix3& rhs) const noexcept
        {
            Matrix3 result{};
            for (int i = 0; i < 3; ++i)
                for (int j = 0; j < 3; ++j)
                    result.m[i][j] = m[i][0] * rhs.m[0][j] + m[i][1] * rhs.m[1][j] + m[i][2] * rhs.m[2][j];
            return result;
        }

        [[nodiscard]] constexpr Vector3 operator*(const Vector3& v) const noexcept
        {
            return {
                m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z,
                m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z,
                m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z
            };
        }

        
        
        
        [[nodiscard]] float Determinant() const noexcept
        {
            return
                m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) -
                m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) +
                m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
        }

        [[nodiscard]] Matrix3 Inverse() const noexcept
        {
            float det = Determinant();
            if (std::fabs(det) < 1e-6f)
                return Identity();

            float invDet = 1.0f / det;

            Matrix3 r;
            r.m[0][0] = (m[1][1] * m[2][2] - m[1][2] * m[2][1]) * invDet;
            r.m[0][1] = -(m[0][1] * m[2][2] - m[0][2] * m[2][1]) * invDet;
            r.m[0][2] = (m[0][1] * m[1][2] - m[0][2] * m[1][1]) * invDet;

            r.m[1][0] = -(m[1][0] * m[2][2] - m[1][2] * m[2][0]) * invDet;
            r.m[1][1] = (m[0][0] * m[2][2] - m[0][2] * m[2][0]) * invDet;
            r.m[1][2] = -(m[0][0] * m[1][2] - m[0][2] * m[1][0]) * invDet;

            r.m[2][0] = (m[1][0] * m[2][1] - m[1][1] * m[2][0]) * invDet;
            r.m[2][1] = -(m[0][0] * m[2][1] - m[0][1] * m[2][0]) * invDet;
            r.m[2][2] = (m[0][0] * m[1][1] - m[0][1] * m[1][0]) * invDet;
            return r;
        }

        
        
        
        [[nodiscard]] static Matrix3 Translation(float tx, float ty) noexcept
        {
            return Matrix3(
                1, 0, tx,
                0, 1, ty,
                0, 0, 1
            );
        }

        [[nodiscard]] static Matrix3 Rotation(float degrees) noexcept
        {
            const float rad = degrees * (3.1415926535f / 180.f);
            const float c = std::cos(rad);
            const float s = std::sin(rad);
            return Matrix3(
                c, -s, 0,
                s, c, 0,
                0, 0, 1
            );
        }

        [[nodiscard]] static Matrix3 Scale(float sx, float sy) noexcept
        {
            return Matrix3(
                sx, 0, 0,
                0, sy, 0,
                0, 0, 1
            );
        }
    };
}
