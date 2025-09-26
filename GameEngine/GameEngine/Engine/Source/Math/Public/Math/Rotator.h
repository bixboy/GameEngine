#pragma once
#include <cmath>
#include <string>

#include "Math/Vector3.h"

namespace Engine::Math
{
    struct Rotator
    {
        float pitch{0.0f};
        float yaw{0.0f};
        float roll{0.0f};

        static constexpr float kDegreesToRadians = 0.0174532925199432957692369077f;

        constexpr Rotator() = default;
        constexpr Rotator(float pitchDegrees, float yawDegrees, float rollDegrees)
            : pitch(pitchDegrees), yaw(yawDegrees), roll(rollDegrees) {}

        [[nodiscard]] static constexpr Rotator Zero() noexcept { return Rotator(0.0f, 0.0f, 0.0f); }

        [[nodiscard]] float PitchRad() const noexcept { return pitch * kDegreesToRadians; }
        [[nodiscard]] float YawRad() const noexcept   { return yaw * kDegreesToRadians; }
        [[nodiscard]] float RollRad() const noexcept  { return roll * kDegreesToRadians; }

        [[nodiscard]] Vector3 Forward() const noexcept
        {
            const float cp = std::cos(PitchRad());
            const float sp = std::sin(PitchRad());
            const float cy = std::cos(YawRad());
            const float sy = std::sin(YawRad());

            return Vector3(cp * cy, sp, cp * sy);
        }

        [[nodiscard]] Vector3 Right() const noexcept
        {
            const float cy = std::cos(YawRad());
            const float sy = std::sin(YawRad());
            return Vector3(-sy, 0.0f, cy);
        }

        [[nodiscard]] Vector3 Up() const noexcept
        {
            const Vector3 forward = Forward();
            const Vector3 right = Right();
            return right.Cross(forward).Normalized();
        }

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

        [[nodiscard]] std::string ToString() const
        {
            return "Rotator(P=" + std::to_string(pitch) +
                   ", Y=" + std::to_string(yaw) +
                   ", R=" + std::to_string(roll) + ")";
        }
    };
}