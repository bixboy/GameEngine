#pragma once

#include <cmath>
#include <string>

struct Vector2 {
    float x; ///< X component
    float y; ///< Y component

    // --- Constructors ---
    Vector2() : x(0), y(0) {}
    Vector2(float x, float y) : x(x), y(y) {}

    // --- Constant vectors ---
    static Vector2 Zero() { return Vector2(0, 0);  } ///< (0,0)
    static Vector2 One()  { return Vector2(1, 1);  } ///< (1,1)
    static Vector2 Up()   { return Vector2(0, -1); } ///< Up direction (-Y)
    static Vector2 Down() { return Vector2(0, 1);  } ///< Down direction (+Y)
    static Vector2 Left() { return Vector2(-1, 0); } ///< Left direction (-X)
    static Vector2 Right(){ return Vector2(1, 0);  } ///< Right direction (+X)

    
    // --- Magnitude & normalization ---
    /**
     * @brief Compute the vector's magnitude (length).
     */
    float Length() const
    { return std::sqrt(x * x + y * y); }

    
    /**
     * @brief Compute the squared length (avoids sqrt for performance).
     */
    float LengthSquared() const
    { return x * x + y * y; }


    /**
     * @brief Returns a normalized version of this vector (length = 1).
     */
    Vector2 Normalized() const {
        float len = Length();
        return len > 0 ? Vector2(x / len, y / len) : Vector2();
    }

    
    // --- Dot & Cross products ---
    /**
     * @brief Compute the dot product with another vector.
     */
    float Dot(const Vector2& other) const
    { return x * other.x + y * other.y; }

    
    /**
     * @brief Compute the cross product (scalar in 2D).
     * Positive result means 'other' is to the left of this vector.
     */
    float Cross(const Vector2& other) const { return x * other.y - y * other.x; }

    // --- Arithmetic operators ---
    
    /// Add two vectors
    Vector2 operator+(const Vector2& other) const
    { return Vector2(x + other.x, y + other.y); }
    
    /// Subtract two vectors
    Vector2 operator-(const Vector2& other) const
    { return Vector2(x - other.x, y - other.y); }
    
    /// Multiply vector by scalar
    Vector2 operator*(float scalar) const
    { return Vector2(x * scalar, y * scalar); }
    
    /// Divide vector by scalar
    Vector2 operator/(float scalar) const
    { return Vector2(x / scalar, y / scalar); }

    Vector2& operator+=(const Vector2& other) { x += other.x; y += other.y; return *this; }
    Vector2& operator-=(const Vector2& other) { x -= other.x; y -= other.y; return *this; }
    Vector2& operator*=(float scalar) { x *= scalar; y *= scalar; return *this; }
    Vector2& operator/=(float scalar) { x /= scalar; y /= scalar; return *this; }

    
    // --- Comparison operators ---
    bool operator==(const Vector2& other) const
    { return x == other.x && y == other.y; }
    
    bool operator!=(const Vector2& other) const
    { return !(*this == other); }

    
    // --- Debugging ---
    /**
     * @brief Convert vector to string for debugging.
     * @return A string like "Vector2(3.000000, 4.000000)"
     */
    std::string ToString() const {
        return "Vector2(" + std::to_string(x) + ", " + std::to_string(y) + ")";
    }
};
