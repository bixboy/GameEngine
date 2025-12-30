#pragma once
#include <algorithm> // Pour std::clamp
#include <cmath>     // Pour std::sin, std::cos, std::fmod, std::abs
#include "Containers/String.h"
#include "Math/Vector3.h"

namespace BixEngine::Math
{
    struct Rotator
    {
        float pitch{0.0f};
        float yaw{0.0f};
        float roll{0.0f};

        static constexpr float kDegreesToRadians = 0.0174532925199432957692369077f;
        static constexpr float kRadiansToDegrees = 57.2957795130823208767981548141f;
        static constexpr float kFullRotation = 360.0f;

        // --- Constructeurs ---
        
        constexpr Rotator() = default;
        constexpr Rotator(float _pitch, float _yaw, float _roll) : pitch(_pitch), yaw(_yaw), roll(_roll) {}
        
        explicit Rotator(float uniform) : pitch(uniform), yaw(uniform), roll(uniform) {}

        // --- Helpers Statiques ---
        
        [[nodiscard]] static constexpr Rotator Zero() noexcept
        {
            return Rotator(0.0f, 0.0f, 0.0f);
        }

        [[nodiscard]] static constexpr Rotator FromVector(const Vector3& eulerDegrees) noexcept
        {
            return Rotator(eulerDegrees.x, eulerDegrees.y, eulerDegrees.z);
        }

        [[nodiscard]] static Rotator FromRadians(const Vector3& radians) noexcept
        {
            return Rotator(radians.x * kRadiansToDegrees,
                           radians.y * kRadiansToDegrees,
                           radians.z * kRadiansToDegrees);
        }

        // --- Conversions ---
        [[nodiscard]] float PitchRad() const noexcept { return pitch * kDegreesToRadians; }
        [[nodiscard]] float YawRad() const noexcept { return yaw * kDegreesToRadians; }
        [[nodiscard]] float RollRad() const noexcept { return roll * kDegreesToRadians; }

        [[nodiscard]] Vector3 ToEuler() const noexcept { return Vector3(pitch, yaw, roll); }

        // --- Vecteurs de Direction (Forward, Right, Up) ---
        
        [[nodiscard]] Vector3 GetForwardVector() const noexcept
        {
            const float cp = std::cos(PitchRad());
            const float sp = std::sin(PitchRad());
            const float cy = std::cos(YawRad());
            const float sy = std::sin(YawRad());

            return Vector3(sy * cp, sp, cy * cp);
        }
        
        [[nodiscard]] Vector3 GetRightVector() const noexcept
        {
            return Vector3::Cross(Vector3::Up(), GetForwardVector()).Normalized();
        }
        
        [[nodiscard]] Vector3 GetUpVector() const noexcept
        {
            return Vector3::Cross(GetForwardVector(), GetRightVector());
        }

        // --- Opérateurs Arithmétiques ---

        [[nodiscard]] constexpr Rotator operator+(const Rotator& other) const noexcept
        {
            return Rotator(pitch + other.pitch, yaw + other.yaw, roll + other.roll);
        }

        [[nodiscard]] constexpr Rotator operator-(const Rotator& other) const noexcept
        {
            return Rotator(pitch - other.pitch, yaw - other.yaw, roll - other.roll);
        }

        [[nodiscard]] constexpr Rotator operator*(float scalar) const noexcept
        {
            return Rotator(pitch * scalar, yaw * scalar, roll * scalar);
        }

        // --- Opérateurs Composés ---

        Rotator& operator+=(const Rotator& other) noexcept
        {
            pitch += other.pitch; yaw += other.yaw; roll += other.roll;
            return *this;
        }

        Rotator& operator-=(const Rotator& other) noexcept
        {
            pitch -= other.pitch; yaw -= other.yaw; roll -= other.roll;
            return *this;
        }
        

        [[nodiscard]] bool IsNearlyZero(float tolerance = 1e-4f) const noexcept
        {
            return std::abs(NormalizeAxis(pitch)) <= tolerance &&
                   std::abs(NormalizeAxis(yaw)) <= tolerance &&
                   std::abs(NormalizeAxis(roll)) <= tolerance;
        }

        void Normalize() noexcept
        {
            pitch = NormalizeAxis(pitch);
            yaw = NormalizeAxis(yaw);
            roll = NormalizeAxis(roll);
        }

        [[nodiscard]] Rotator Normalized() const noexcept
        {
            Rotator result(*this);
            result.Normalize();
            return result;
        }
        
        [[nodiscard]] Rotator Clamp(float minPitch, float maxPitch) const noexcept
        {
             Rotator r = *this;
             r.pitch = std::clamp(r.pitch, minPitch, maxPitch);
            
             return r;
        }

        [[nodiscard]] String ToString() const
        {
            return String("Rotator(P=") + std::to_string(pitch) + ", Y=" + std::to_string(yaw) + ", R=" + std::to_string(roll) + ")";
        }

    private:
        static float NormalizeAxis(float angle) noexcept
        {
            angle = std::fmod(angle, kFullRotation);
            if (angle < -180.0f) angle += kFullRotation;
            if (angle > 180.0f) angle -= kFullRotation;
            return angle;
        }
    };
}