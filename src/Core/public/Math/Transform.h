#pragma once
#include "Math/Vector3.h"
#include "Math/Rotator.h"
#include "Matrix/Matrix3.h"


namespace BixEngine::Math
{
    constexpr float Deg2Rad(float deg) { return deg * (3.1415926535f / 180.f); }
    constexpr float Rad2Deg(float rad) { return rad * (180.f / 3.1415926535f); }

    struct Transform
    {
        Vector3 position{Vector3::Zero()};
        Rotator rotation{Rotator::Zero()};
        Vector3 scale{Vector3::One()};

        Transform* parent = nullptr;

        Transform() = default;

        Transform(const Vector3& pos, const Rotator& rot, const Vector3& scl) : position(pos), rotation(rot), scale(scl)
        {
        }

        // ────────────────────────────────────────────────
        // Fonctions de manipulation
        // ────────────────────────────────────────────────
        void Translate(const Vector3& delta) { position += delta; }

        Vector3 GetLocalPosition() const { return position; }
        
        Vector3 GetWorldPosition() const
        {
            if (parent)
            {
                return TransformPoint(Vector3::Zero());
            }
            return position;
        }

        Vector3 GetPosition() { return GetWorldPosition(); } // CHANGING BEHAVIOR TO WORLD FOR SAFETY IF USER EXPECTS THAT


        void SetPosition(const Vector3 NewPosition) { position = NewPosition; } 

        void Rotate(const Rotator& delta)
        {
            rotation.pitch += delta.pitch;
            rotation.yaw += delta.yaw;
            rotation.roll += delta.roll;
        }

        void ScaleBy(const Vector3& factor)
        {
            scale.x *= factor.x;
            scale.y *= factor.y;
            scale.z *= factor.z;
        }

        void SetParent(Transform* newParent) { parent = newParent; }

        [[nodiscard]] Matrix3 ToMatrix3() const noexcept
        {
            Matrix3 translation = Matrix3::Translation(position.x, position.y);
            Matrix3 rotationM = Matrix3::Rotation(rotation.yaw);
            Matrix3 scaleM = Matrix3::Scale(scale.x, scale.y);

            Matrix3 local = translation * rotationM * scaleM;

            if (parent)
                local = parent->ToMatrix3() * local;

            return local;
        }

        // ────────────────────────────────────────────────
        // Application des transformations
        // ────────────────────────────────────────────────
        [[nodiscard]] Vector3 TransformPoint(const Vector3& localPoint) const noexcept
        {
            Vector3 input(localPoint.x, localPoint.y, 1.0f);
            Vector3 result = ToMatrix3() * input;

            return {result.x, result.y, 0.0f};
        }
    };
}
