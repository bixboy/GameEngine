#pragma once
#include <cmath>
#include "Containers/String.h"


namespace BixEngine::Math
{
    template <typename T>
        requires (std::same_as<T, int> || std::same_as<T, float>)

    struct Vector2
    {
        T x{0};
        T y{0};

        constexpr Vector2() = default;

        constexpr Vector2(T xValue, T yValue) : x(xValue), y(yValue)
        {
        }

        [[nodiscard]] static constexpr Vector2 Zero() noexcept { return Vector2(0, 0); }
        [[nodiscard]] static constexpr Vector2 One() noexcept { return Vector2(1, 1); }
        [[nodiscard]] static constexpr Vector2 Up() noexcept { return Vector2(0, -1); }
        [[nodiscard]] static constexpr Vector2 Down() noexcept { return Vector2(0, 1); }
        [[nodiscard]] static constexpr Vector2 Left() noexcept { return Vector2(-1, 0); }
        [[nodiscard]] static constexpr Vector2 Right() noexcept { return Vector2(1, 0); }

        [[nodiscard]] T Length() const noexcept { return std::sqrt(x * x + y * y); }
        [[nodiscard]] T LengthSquared() const noexcept { return x * x + y * y; }

        [[nodiscard]] Vector2 Normalized() const noexcept
        {
            const T len = Length();
            return len > 0 ? Vector2(x / len, y / len) : Vector2();
        }

        [[nodiscard]] T Dot(const Vector2& other) const noexcept { return x * other.x + y * other.y; }
        [[nodiscard]] T Cross(const Vector2& other) const noexcept { return x * other.y - y * other.x; }

        [[nodiscard]] constexpr Vector2 operator+(const Vector2& other) const noexcept
        {
            return Vector2(x + other.x, y + other.y);
        }

        [[nodiscard]] constexpr Vector2 operator-(const Vector2& other) const noexcept
        {
            return Vector2(x - other.x, y - other.y);
        }

        [[nodiscard]] constexpr Vector2 operator*(T scalar) const noexcept
        {
            return Vector2(x * scalar, y * scalar);
        }

        [[nodiscard]] constexpr Vector2 operator/(T scalar) const noexcept
        {
            return Vector2(x / scalar, y / scalar);
        }

        Vector2& operator+=(const Vector2& other) noexcept
        {
            x += other.x;
            y += other.y;
            return *this;
        }

        Vector2& operator-=(const Vector2& other) noexcept
        {
            x -= other.x;
            y -= other.y;
            return *this;
        }

        Vector2& operator*=(T scalar) noexcept
        {
            x *= scalar;
            y *= scalar;
            return *this;
        }

        Vector2& operator/=(T scalar) noexcept
        {
            x /= scalar;
            y /= scalar;
            return *this;
        }

        [[nodiscard]] constexpr bool operator==(const Vector2& other) const noexcept
        {
            return x == other.x && y == other.y;
        }

        [[nodiscard]] constexpr bool operator!=(const Vector2& other) const noexcept
        {
            return !(*this == other);
        }

        [[nodiscard]] String ToString() const
        {
            return String("Vector2(") + std::to_string(x) + ", " + std::to_string(y) + ")";
        }
    };
}
