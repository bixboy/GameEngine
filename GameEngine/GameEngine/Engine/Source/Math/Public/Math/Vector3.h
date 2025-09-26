#pragma once
#include <cmath>
#include <string>

namespace Engine::Math
{
    struct Vector3
    {
        float x{0.0f};
        float y{0.0f};
        float z{0.0f};

        constexpr Vector3() = default;
        constexpr Vector3(float xValue, float yValue, float zValue) : x(xValue), y(yValue), z(zValue) {}

        [[nodiscard]] static constexpr Vector3 Zero() noexcept     { return Vector3(0.0f, 0.0f, 0.0f);  }
        [[nodiscard]] static constexpr Vector3 One() noexcept      { return Vector3(1.0f, 1.0f, 1.0f);  }
        [[nodiscard]] static constexpr Vector3 Up() noexcept       { return Vector3(0.0f, 1.0f, 0.0f);  }
        [[nodiscard]] static constexpr Vector3 Down() noexcept     { return Vector3(0.0f, -1.0f, 0.0f); }
        [[nodiscard]] static constexpr Vector3 Forward() noexcept  { return Vector3(0.0f, 0.0f, 1.0f);  }
        [[nodiscard]] static constexpr Vector3 Backward() noexcept { return Vector3(0.0f, 0.0f, -1.0f); }

        [[nodiscard]] float Length() const noexcept { return std::sqrt(x * x + y * y + z * z); }

        [[nodiscard]] Vector3 Normalized() const noexcept
        {
            const float len = Length();
            return len > 0.0f ? Vector3(x / len, y / len, z / len) : Vector3();
        }

        [[nodiscard]] float Dot(const Vector3& other) const noexcept
        {
            return x * other.x + y * other.y + z * other.z;
        }

        [[nodiscard]] Vector3 Cross(const Vector3& other) const noexcept
        {
            return Vector3(
                y * other.z - z * other.y,
                z * other.x - x * other.z,
                x * other.y - y * other.x);
        }

        [[nodiscard]] constexpr Vector3 operator+(const Vector3& other) const noexcept
        {
            return Vector3(x + other.x, y + other.y, z + other.z);
        }

        [[nodiscard]] constexpr Vector3 operator-(const Vector3& other) const noexcept
        {
            return Vector3(x - other.x, y - other.y, z - other.z);
        }

        [[nodiscard]] constexpr Vector3 operator*(float scalar) const noexcept
        {
            return Vector3(x * scalar, y * scalar, z * scalar);
        }

        [[nodiscard]] constexpr Vector3 operator/(float scalar) const noexcept
        {
            return Vector3(x / scalar, y / scalar, z / scalar);
        }

        [[nodiscard]] constexpr bool operator==(const Vector3& other) const noexcept
        {
            return x == other.x && y == other.y && z == other.z;
        }

        [[nodiscard]] constexpr bool operator!=(const Vector3& other) const noexcept
        {
            return !(*this == other);
        }

        [[nodiscard]] std::string ToString() const
        {
            return "Vector3(" + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) + ")";
        }
    };

}
