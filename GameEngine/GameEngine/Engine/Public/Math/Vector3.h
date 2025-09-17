#pragma once
#include <cmath>
#include <string>


struct Vector3 {
    float x, y, z; ///< Vector components

    // --- Constructors ---
    Vector3() : x(0), y(0), z(0) {}
    Vector3(float x, float y, float z) : x(x), y(y), z(z) {}

    // --- Constant vectors ---
    static Vector3 Zero()     { return Vector3(0,0,0);  } ///< (0,0,0)
    static Vector3 One()      { return Vector3(1,1,1);  } ///< (1,1,1)
    static Vector3 Up()       { return Vector3(0,1,0);  } ///< Up direction (+Y)
    static Vector3 Down()     { return Vector3(0,-1,0); } ///< Down direction (-Y)
    static Vector3 Forward()  { return Vector3(0,0,1);  } ///< Forward direction (+Z)
    static Vector3 Backward() { return Vector3(0,0,-1); } ///< Backward direction (-Z)


    // --- Magnitude & normalization ---
    /**
     * @brief Compute the vector's magnitude (length).
     */
    float Length() const
    { return std::sqrt(x*x + y*y + z*z); }

    
    /**
     * @brief Returns a normalized version of this vector (length = 1).
     */
    Vector3 Normalized() const {
        float len = Length();
        return len > 0 ? Vector3(x/len, y/len, z/len) : Vector3();
    }


    // --- Dot & Cross products ---
    /**
     * @brief Compute the dot product with another vector.
     */
    float Dot(const Vector3& other) const
    { return x*other.x + y*other.y + z*other.z; }

    
    /**
     * @brief Compute the cross product with another vector.
     * @return A new vector perpendicular to both.
     */
    Vector3 Cross(const Vector3& o) const {
        return Vector3(
            y * o.z - z * o.y,
            z * o.x - x * o.z,
            x * o.y - y * o.x
        );
    }

    // Add
    Vector3 operator+(const Vector3& o) const
    { return Vector3(x+o.x, y+o.y, z+o.z); }

    // Subtract
    Vector3 operator-(const Vector3& o) const
    { return Vector3(x-o.x, y-o.y, z-o.z); }

    // Multiply
    Vector3 operator*(float s) const
    { return Vector3(x*s, y*s, z*s); }

    // Divide
    Vector3 operator/(float s) const
    { return Vector3(x/s, y/s, z/s); }


    // --- Comparison operators ---
    bool operator==(const Vector3& other) const
    { return x == other.x && y == other.y && z == other.z; }
    
    bool operator!=(const Vector3& other) const
    { return !(*this == other); }


    // --- Debugging ---
    /**
     * @brief Convert vector to string for debugging.
     * @return A string like "Vector3(1.000000, 0.000000, 2.000000)"
     */
    std::string ToString() const {
        return "Vector3(" + std::to_string(x) + ", "
                          + std::to_string(y) + ", "
                          + std::to_string(z) + ")";
    }
};
