#pragma once
#include "Math/Vector2.h"


namespace BixEngine::Math
{
    struct Rect
    {
        float X = 0.f;
        float Y = 0.f;
        float Width = 0.f;
        float Height = 0.f;

        constexpr Rect() = default;

        constexpr Rect(float x, float y, float width, float height) : X(x), Y(y), Width(width), Height(height)
        {
        }

        [[nodiscard]] constexpr float GetLeft() const noexcept { return X; }
        [[nodiscard]] constexpr float GetRight() const noexcept { return X + Width; }
        [[nodiscard]] constexpr float GetTop() const noexcept { return Y; }
        [[nodiscard]] constexpr float GetBottom() const noexcept { return Y + Height; }

        [[nodiscard]] constexpr Vector2<float> GetCenter() const noexcept
        {
            return {X + Width * 0.5f, Y + Height * 0.5f};
        }

        [[nodiscard]] constexpr bool Contains(float px, float py) const noexcept
        {
            return px >= X && px <= X + Width && py >= Y && py <= Y + Height;
        }

        [[nodiscard]] constexpr bool Contains(const Vector2<float>& point) const noexcept
        {
            return Contains(point.x, point.y);
        }

        [[nodiscard]] constexpr bool Intersects(const Rect& other) const noexcept
        {
            return !(other.GetLeft() > GetRight() ||
                other.GetRight() < GetLeft() ||
                other.GetTop() > GetBottom() ||
                other.GetBottom() < GetTop());
        }
    };
}
