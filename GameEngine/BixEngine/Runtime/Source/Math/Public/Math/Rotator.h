#pragma once
#include <algorithm>
#include <cmath>

#include "Core/String.h"

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

        constexpr Rotator() = default;
        constexpr Rotator(float pitchDegrees, float yawDegrees, float rollDegrees)
            : pitch(pitchDegrees), yaw(yawDegrees), roll(rollDegrees) {}

        [[nodiscard]] static constexpr Rotator Zero() noexcept { return Rotator(0.0f, 0.0f, 0.0f); }
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

        [[nodiscard]] float PitchRad() const noexcept { return pitch * kDegreesToRadians; }
        [[nodiscard]] float YawRad() const noexcept   { return yaw * kDegreesToRadians; }
        [[nodiscard]] float RollRad() const noexcept  { return roll * kDegreesToRadians; }

        [[nodiscard]] Vector3 Euler() const noexcept { return Vector3(pitch, yaw, roll); }
        [[nodiscard]] Vector3 EulerRadians() const noexcept { return Vector3(PitchRad(), YawRad(), RollRad()); }

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

        Rotator& operator+=(const Rotator& other) noexcept
        {
            pitch += other.pitch;
            yaw += other.yaw;
            roll += other.roll;
            return *this;
        }

        Rotator& operator-=(const Rotator& other) noexcept
        {
            pitch -= other.pitch;
            yaw -= other.yaw;
            roll -= other.roll;
            return *this;
        }

        Rotator& operator*=(float scalar) noexcept
        {
            pitch *= scalar;
            yaw *= scalar;
            roll *= scalar;
            return *this;
        }

        [[nodiscard]] bool IsNearlyZero(float tolerance = 1e-4f) const noexcept
        {
            return std::abs(NormalizeAngle(pitch)) <= tolerance &&
                   std::abs(NormalizeAngle(yaw)) <= tolerance &&
                   std::abs(NormalizeAngle(roll)) <= tolerance;
        }

        [[nodiscard]] bool Equals(const Rotator& other, float tolerance = 1e-4f) const noexcept
        {
            return (*this - other).Normalized().IsNearlyZero(tolerance);
        }

        [[nodiscard]] Rotator Normalized() const noexcept
        {
            Rotator result(*this);
            result.Normalize();
            return result;
        }

        void Normalize() noexcept
        {
            pitch = NormalizeAngle(pitch);
            yaw = NormalizeAngle(yaw);
            roll = NormalizeAngle(roll);
        }

        [[nodiscard]] Rotator Clamp(float minAngle = -180.0f, float maxAngle = 180.0f) const noexcept
        {
            Rotator result(*this);
            result.ClampInline(minAngle, maxAngle);
            return result;
        }

        void ClampInline(float minAngle = -180.0f, float maxAngle = 180.0f) noexcept
        {
            pitch = std::clamp(pitch, minAngle, maxAngle);
            yaw   = std::clamp(yaw,   minAngle, maxAngle);
            roll  = std::clamp(roll,  minAngle, maxAngle);
        }

        Rotator& Add(float deltaPitch, float deltaYaw, float deltaRoll) noexcept
        {
            pitch += deltaPitch;
            yaw += deltaYaw;
            roll += deltaRoll;
            return *this;
        }

        [[nodiscard]] Rotator Inverse() const noexcept
        {
            return Rotator(-pitch, -yaw, -roll);
        }

        [[nodiscard]] String ToString() const
        {
            return String("Rotator(P=") + std::to_string(pitch) +
                   ", Y=" + std::to_string(yaw) +
                   ", R=" + std::to_string(roll) + ")";
        }

    private:
        static float NormalizeAngle(float angle) noexcept
        {
            float result = std::fmod(angle, kFullRotation);
            if (result < -180.0f)
                result += kFullRotation;
            else if (result > 180.0f)
                result -= kFullRotation;
            return result;
        }
    };
}
