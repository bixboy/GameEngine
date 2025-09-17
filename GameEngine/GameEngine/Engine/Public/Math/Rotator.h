#pragma once
#include "Vector3.h"

struct Rotator {
    float pitch; ///< Rotation around X-axis (looking up/down)
    float yaw;   ///< Rotation around Y-axis (turning left/right)
    float roll;  ///< Rotation around Z-axis (tilting head)

    // --- Constructors ---
    Rotator() : pitch(0), yaw(0), roll(0) {}
    Rotator(float p, float y, float r) : pitch(p), yaw(y), roll(r) {}

    // --- Constants ---
    static Rotator Zero()
    { return Rotator(0, 0, 0); }
    

    // --- Conversion to radians ---
    float PitchRad() const
    { return pitch * (3.1415926535f / 180.0f); }
    
    float YawRad()   const
    { return yaw   * (3.1415926535f / 180.0f); }
    
    float RollRad()  const
    { return roll  * (3.1415926535f / 180.0f); }
    

    // --- Direction vectors ---
    /**
     * @brief Returns the forward direction vector (based on Pitch & Yaw).
     * Equivalent to "look direction".
     */
    Vector3 Forward() const {
        float cp = cosf(PitchRad());
        float sp = sinf(PitchRad());
        float cy = cosf(YawRad());
        float sy = sinf(YawRad());

        return Vector3(cp * cy, sp, cp * sy);
    }

    /**
     * @brief Returns the right direction vector (perpendicular to Forward).
     */
    Vector3 Right() const {
        float cy = cosf(YawRad());
        float sy = sinf(YawRad());

        return Vector3(-sy, 0, cy);
    }

    /**
     * @brief Returns the up direction vector (based on Pitch, Yaw, Roll).
     */
    Vector3 Up() const {
        Vector3 f = Forward();
        Vector3 r = Right();
        return r.Cross(f).Normalized();
    }

    // --- Operators ---
    Rotator operator+(const Rotator& other) const
    { return Rotator(pitch + other.pitch, yaw + other.yaw, roll + other.roll); }
    
    Rotator operator-(const Rotator& other) const
    { return Rotator(pitch - other.pitch, yaw - other.yaw, roll - other.roll); }
    
    Rotator operator*(float scalar) const
    { return Rotator(pitch * scalar, yaw * scalar, roll * scalar); }

    
    // --- ToString ---
    std::string ToString() const {
        return "Rotator(P=" + std::to_string(pitch) +
               ", Y=" + std::to_string(yaw) +
               ", R=" + std::to_string(roll) + ")";
    }
};
