#pragma once
#include <cmath>
#include <cassert>
#include "Containers/String.h"


namespace BixEngine::Math
{
    template <typename T>
    requires std::is_arithmetic_v<T> 
    struct TVector2
    {
        T x{0};
        T y{0};

        constexpr TVector2() = default;
        constexpr TVector2(T xValue, T yValue) : x(xValue), y(yValue) {}

        [[nodiscard]] static constexpr TVector2 Zero() noexcept { return TVector2(0, 0); }
        [[nodiscard]] static constexpr TVector2 One() noexcept { return TVector2(1, 1); }
        
        [[nodiscard]] static constexpr TVector2 Up() noexcept { return TVector2(0, -1); }
        [[nodiscard]] static constexpr TVector2 Down() noexcept { return TVector2(0, 1); }
        [[nodiscard]] static constexpr TVector2 Left() noexcept { return TVector2(-1, 0); }
        [[nodiscard]] static constexpr TVector2 Right() noexcept { return TVector2(1, 0); }

        [[nodiscard]] auto Length() const noexcept 
        { 
            if constexpr (std::is_same_v<T, double>)
            {
                return std::sqrt(x * x + y * y);
            }
            else
                return std::sqrt(static_cast<float>(x * x + y * y));
        }
        
        [[nodiscard]] T LengthSquared() const noexcept { return x * x + y * y; }

        [[nodiscard]] TVector2 Normalized() const noexcept
        {
            auto len = Length();
            if (len < 1e-5) return Zero();
            
            T scale = static_cast<T>(1.0 / len);
            return TVector2(static_cast<T>(x * scale), static_cast<T>(y * scale));
        }

        [[nodiscard]] T Dot(const TVector2& other) const noexcept { return x * other.x + y * other.y; }
        
        [[nodiscard]] T Cross(const TVector2& other) const noexcept { return x * other.y - y * other.x; }

        // --- Opérateurs Composés ---
        
        TVector2& operator += (const TVector2& other) noexcept { x += other.x; y += other.y; return *this; }
        TVector2& operator -= (const TVector2& other) noexcept { x -= other.x; y -= other.y; return *this; }
        TVector2& operator *= (const TVector2& other) noexcept { x *= other.x; y *= other.y; return *this; }
        TVector2& operator *= (T scalar) noexcept { x *= scalar; y *= scalar; return *this; }
        
        TVector2& operator/=(T scalar) noexcept
        {
            assert(std::abs(scalar) > 1e-6f && "Division par zero dans Vector2");
            
            T inv = static_cast<T>(1.0) / scalar;
            x *= inv; y *= inv;
            
            return *this;
        }

        // --- Opérateurs Arithmétiques ---
        
        [[nodiscard]] constexpr TVector2 operator-() const noexcept { return TVector2(-x, -y); }

        [[nodiscard]] constexpr TVector2 operator+(const TVector2& other) const noexcept { return TVector2(*this) += other; }
        [[nodiscard]] constexpr TVector2 operator-(const TVector2& other) const noexcept { return TVector2(*this) -= other; }
        
        // Multi Vecteur * Vecteur (Scale)
        [[nodiscard]] constexpr TVector2 operator*(const TVector2& other) const noexcept { return TVector2(*this) *= other; }
        
        // Multi Vecteur * Scalaire
        [[nodiscard]] constexpr TVector2 operator*(T scalar) const noexcept { return TVector2(*this) *= scalar; }
        
        // Division Vecteur / Scalaire
        [[nodiscard]] constexpr TVector2 operator/(T scalar) const noexcept { return TVector2(*this) /= scalar; }

        // Comparaison
        [[nodiscard]] constexpr bool operator==(const TVector2& other) const noexcept { return x == other.x && y == other.y; }
        [[nodiscard]] constexpr bool operator!=(const TVector2& other) const noexcept { return !(*this == other); }

        friend TVector2 operator*(T s, const TVector2& v) { return v * s; }

        [[nodiscard]] String ToString() const
        {
            return String("Vector2(") + std::to_string(x) + ", " + std::to_string(y) + ")";
        }
    };
    
    // Alias
    using Vector2 = TVector2<float>;
    
    using Vec2 = TVector2<float>;
    using Vec2i = TVector2<int>;
}