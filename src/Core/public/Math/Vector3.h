#pragma once
#include <cmath>
#include "Containers/String.h"


namespace BixEngine::Math
{
    template <typename T>
    requires std::is_arithmetic_v<T>
    struct TVector3
    {
        T x{0}, y{0}, z{0};

        static constexpr TVector3 Zero() { return {0, 0, 0}; }
        static constexpr TVector3 One() { return {1, 1, 1}; }
        static constexpr TVector3 Up() { return {0, 1, 0}; }
        static constexpr TVector3 Forward() { return {0, 0, 1}; }

        constexpr TVector3() = default;
        constexpr TVector3(T _x, T _y, T _z) : x(_x), y(_y), z(_z) {}
        
        [[nodiscard]] auto LengthSquared() const 
        {
            return x * x + y * y + z * z;
        }

        [[nodiscard]] auto Length() const 
        {
            if constexpr (std::is_same_v<T, double>)
            {
                return std::sqrt(x * x + y * y + z * z);
            }
            else
                return std::sqrt(static_cast<float>(x * x + y * y + z * z));
        }
        
        [[nodiscard]] TVector3 Normalized() const 
        {
            auto len = Length();
            if (len < 1e-5)
                return Zero();
            
            T scale = static_cast<T>(1.0 / len);
            
            return {
                static_cast<T>(x * scale),
                static_cast<T>(y * scale),
                static_cast<T>(z * scale) };
        }

        // --- Opérateurs Composés ---
        
        TVector3& operator+=(const TVector3& o) { x += o.x; y += o.y; z += o.z; return *this; }
        TVector3& operator-=(const TVector3& o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
        TVector3& operator*=(const TVector3& o) { x *= o.x; y *= o.y; z *= o.z; return *this; }
        
        TVector3& operator*=(T s)
        {
            x *= s; y *= s; z *= s; return *this;
        }
        
        TVector3& operator/=(T s)
        { 
            if (std::abs(s) < 1e-6f)
            {
                // Remplace par ton système de log, ex: BixLog::Error("Div par 0 !");
                return *this;
            }
    
            T inv = static_cast<T>(1.0) / s; 
            x *= inv; y *= inv; z *= inv;
    
            return *this; 
        }

        // --- Opérateurs Arithmétiques  ---
        
        TVector3 operator+(const TVector3& o) const { return TVector3(*this) += o; }
        TVector3 operator-(const TVector3& o) const { return TVector3(*this) -= o; }
        TVector3 operator*(const TVector3& o) const { return TVector3(*this) *= o; }

        TVector3 operator*(T s) const { return TVector3(*this) *= s; }
        
        TVector3 operator/(T s) const { return TVector3(*this) /= s; }

        bool operator==(const TVector3& o) const { return x == o.x && y == o.y && z == o.z; }
        bool operator!=(const TVector3& o) const { return !(*this == o); }

        friend TVector3 operator*(T s, const TVector3& v) { return v * s; }

        
        static T Dot(const TVector3& a, const TVector3& b)
        {
            return a.x * b.x + a.y * b.y + a.z * b.z;
        }
        
        static TVector3 Cross(const TVector3& a, const TVector3& b)
        {
            return {
                a.y * b.z - a.z * b.y,
                a.z * b.x - a.x * b.z,
                a.x * b.y - a.y * b.x };
        }
        
        static TVector3 Lerp(const TVector3& a, const TVector3& b, float t)
        {
            return a + (b - a) * std::clamp(t, 0.0f, 1.0f);
        }

        [[nodiscard]] T Dot(const TVector3& other) const
        {
            return Dot(*this, other);
        }

        [[nodiscard]] TVector3 Cross(const TVector3& other) const
        {
            return Cross(*this, other);
        }

        [[nodiscard]] String ToString() const
        {
            return String("Vector3(") + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) + ")";
        }
    };

    // Alias
    using Vector3 = TVector3<float>;
    
    using Vec3 = TVector3<float>;
    using Vec3i = TVector3<int>;
}