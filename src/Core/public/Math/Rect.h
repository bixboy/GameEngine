#pragma once
#include "Math/Vector2.h"
#include <algorithm> // Pour std::min, std::max
#include "Containers/String.h"

namespace BixEngine::Math
{
    template <typename T>
    requires std::is_arithmetic_v<T>
    struct TRect
    {
        T x{0};
        T y{0};
        T width{0};
        T height{0};

        constexpr TRect() = default;

        constexpr TRect(T _x, T _y, T _width, T _height) 
            : x(_x), y(_y), width(_width), height(_height) 
        {}

        constexpr TRect(const TVector2<T>& position, const TVector2<T>& size) : x(position.x), y(position.y), width(size.x), height(size.y)
        {}

        [[nodiscard]] static constexpr TRect Zero() noexcept { return TRect(0, 0, 0, 0); }

        [[nodiscard]] constexpr T GetLeft() const noexcept { return x; }
        [[nodiscard]] constexpr T GetRight() const noexcept { return x + width; }
        [[nodiscard]] constexpr T GetTop() const noexcept { return y; }
        [[nodiscard]] constexpr T GetBottom() const noexcept { return y + height; }

        [[nodiscard]] constexpr TVector2<T> GetPosition() const noexcept { return {x, y}; }
        [[nodiscard]] constexpr TVector2<T> GetSize() const noexcept { return {width, height}; }
        
        [[nodiscard]] constexpr TVector2<T> GetCenter() const noexcept
        {
            return {x + width / static_cast<T>(2), y + height / static_cast<T>(2)};
        }
        
        [[nodiscard]] constexpr bool Contains(T px, T py) const noexcept
        {
            return px >= x && px <= (x + width) && py >= y && py <= (y + height);
        }

        [[nodiscard]] constexpr bool Contains(const TVector2<T>& point) const noexcept
        {
            return Contains(point.x, point.y);
        }

        [[nodiscard]] constexpr bool Contains(const TRect<T>& other) const noexcept
        {
            return other.x >= x && other.GetRight() <= GetRight() && other.y >= y && other.GetBottom() <= GetBottom();
        }

        [[nodiscard]] constexpr bool Intersects(const TRect& other) const noexcept
        {
            return !(other.GetLeft() > GetRight() ||
                     other.GetRight() < GetLeft() ||
                     other.GetTop() > GetBottom() ||
                     other.GetBottom() < GetTop());
        }
        
        void Encapsulate(const TVector2<T>& point) noexcept
        {
            T right = std::max(GetRight(), point.x);
            T bottom = std::max(GetBottom(), point.y);
            
            x = std::min(x, point.x);
            y = std::min(y, point.y);
            
            width = right - x;
            height = bottom - y;
        }
        
        [[nodiscard]] TRect GetIntersection(const TRect& other) const noexcept
        {
            T interLeft = std::max(x, other.x);
            T interTop = std::max(y, other.y);
            T interRight = std::min(GetRight(), other.GetRight());
            T interBottom = std::min(GetBottom(), other.GetBottom());

            if (interLeft < interRight && interTop < interBottom)
            {
                return TRect(interLeft, interTop, interRight - interLeft, interBottom - interTop);
            }
            
            return Zero();
        }

        // --- Opérateurs ---

        [[nodiscard]] bool operator==(const TRect& other) const noexcept
        {
            return x == other.x && y == other.y && width == other.width && height == other.height;
        }

        [[nodiscard]] bool operator!=(const TRect& other) const noexcept
        {
            return !(*this == other);
        }

        [[nodiscard]] String ToString() const
        {
            return String("Rect(X=") + std::to_string(x) + ", Y=" + std::to_string(y) + ", W=" + std::to_string(width) + ", H=" + std::to_string(height) + ")";
        }
    };

    // Alias
    using Rect = TRect<float>;
    using Rectf = TRect<float>;
    using Recti = TRect<int>;
}