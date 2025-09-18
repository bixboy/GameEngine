#pragma once

#include <cmath>
#include <string>

namespace Engine::Math {

struct Vector2 {
    float x{0.0f};
    float y{0.0f};

    constexpr Vector2() = default;
    constexpr Vector2(float xValue, float yValue) : x(xValue), y(yValue) {}

    [[nodiscard]] static constexpr Vector2 Zero() noexcept { return Vector2(0.0f, 0.0f); }
    [[nodiscard]] static constexpr Vector2 One() noexcept { return Vector2(1.0f, 1.0f); }
    [[nodiscard]] static constexpr Vector2 Up() noexcept { return Vector2(0.0f, -1.0f); }
    [[nodiscard]] static constexpr Vector2 Down() noexcept { return Vector2(0.0f, 1.0f); }
    [[nodiscard]] static constexpr Vector2 Left() noexcept { return Vector2(-1.0f, 0.0f); }
    [[nodiscard]] static constexpr Vector2 Right() noexcept { return Vector2(1.0f, 0.0f); }

    [[nodiscard]] float Length() const noexcept { return std::sqrt(x * x + y * y); }
    [[nodiscard]] float LengthSquared() const noexcept { return x * x + y * y; }

    [[nodiscard]] Vector2 Normalized() const noexcept {
        const float len = Length();
        return len > 0.0f ? Vector2(x / len, y / len) : Vector2();
    }

    [[nodiscard]] float Dot(const Vector2& other) const noexcept { return x * other.x + y * other.y; }
    [[nodiscard]] float Cross(const Vector2& other) const noexcept { return x * other.y - y * other.x; }

    [[nodiscard]] constexpr Vector2 operator+(const Vector2& other) const noexcept {
        return Vector2(x + other.x, y + other.y);
    }

    [[nodiscard]] constexpr Vector2 operator-(const Vector2& other) const noexcept {
        return Vector2(x - other.x, y - other.y);
    }

    [[nodiscard]] constexpr Vector2 operator*(float scalar) const noexcept {
        return Vector2(x * scalar, y * scalar);
    }

    [[nodiscard]] constexpr Vector2 operator/(float scalar) const noexcept {
        return Vector2(x / scalar, y / scalar);
    }

    Vector2& operator+=(const Vector2& other) noexcept {
        x += other.x;
        y += other.y;
        return *this;
    }

    Vector2& operator-=(const Vector2& other) noexcept {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    Vector2& operator*=(float scalar) noexcept {
        x *= scalar;
        y *= scalar;
        return *this;
    }

    Vector2& operator/=(float scalar) noexcept {
        x /= scalar;
        y /= scalar;
        return *this;
    }

    [[nodiscard]] constexpr bool operator==(const Vector2& other) const noexcept {
        return x == other.x && y == other.y;
    }

    [[nodiscard]] constexpr bool operator!=(const Vector2& other) const noexcept {
        return !(*this == other);
    }

    [[nodiscard]] std::string ToString() const {
        return "Vector2(" + std::to_string(x) + ", " + std::to_string(y) + ")";
    }
};

}  // namespace Engine::Math
