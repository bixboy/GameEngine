#pragma once
#include "Math/Vector3.h"
#include "Math/Rotator.h"
#include "Matrix/Matrix4.h"
#include "Matrix/Matrix3.h"
#include <vector>

namespace BixEngine::Math
{
    class Transform
    {
    private:
        Vec3 m_LocalPosition{Vec3::Zero()};
        Rotator m_LocalRotation{Rotator::Zero()};
        Vec3 m_LocalScale{Vec3::One()};

        // Cache system
        mutable Matrix4 m_LocalMatrix;
        mutable Matrix4 m_WorldMatrix;
        mutable bool m_IsDirty = true;

        Transform* m_Parent = nullptr;
        std::vector<Transform*> m_Children;

    public:
        Transform() = default;

        [[nodiscard]] Vec3 GetLocalPosition() const { return m_LocalPosition; }
        [[nodiscard]] Rotator GetLocalRotation() const { return m_LocalRotation; }
        [[nodiscard]] Vec3 GetLocalScale() const { return m_LocalScale; }
        
        void SetPosition(const Vec3& pos)
        { 
            m_LocalPosition = pos; 
            SetDirty(); 
        }

        void SetRotation(const Rotator& rot)
        { 
            m_LocalRotation = rot; 
            SetDirty(); 
        }

        void SetScale(const Vec3& scale)
        { 
            m_LocalScale = scale; 
            SetDirty(); 
        }

        void LookAt(const Vec3& target, const Vec3& up = Vec3::Up())
        {
            // Calculate direction to target
            // Assuming world space calculation for now, but we set local rotation.
            // If parented, we might need to adjust.
            // Simplified implementation:
            // Calculate world rotation needed.
            // If parent, Convert world rotation to local.
            
            Vec3 currentPos = GetWorldPosition();
            Vec3 forward = (target - currentPos).Normalized();
            
            // Handle edge case where target is same as position
            if (forward.LengthSquared() < 0.0001f) return;

            // Create LookAt rotation (Quaternion/Rotator)
            // Assuming Rotator::LookAt or Quaternion::LookRotation exists in the engine math lib.
            // If not, we need to construct it.
            // Let's assume Rotator has a static LookAt or we use Matrix.
            
            // Method 1: Matrix LookAt
            Matrix4 lookAtMatrix = Matrix4::LookAt(currentPos, target, up);
            // Invert because LookAt creates a View Matrix (inverse of camera transform) usually?
            // Wait, Transform usually represents the "Model" matrix.
            // A "LoopAt" for an object means orienting the object Z+ (or forward) towards target.
            // Matrix4::LookAt usually creates a View Matrix (Camera at Eye looking at Target).
            // ViewMatrix = Inverse(CameraTransform).
            // So CameraTransform = Inverse(ViewMatrix).
            
            Matrix4 objectTransform = Matrix4::LookAt(currentPos, target, up).Inverse();
            Rotator worldRot = objectTransform.GetRotation();
            
            if (m_Parent)
            {
                 // Convert worldRot to localRot
                 // World = Parent * Local
                 // Local = Inverse(Parent) * World ?? Rotations add up.
                 // LocalRot = WorldRot - ParentRot ?? Roughly.
                 m_LocalRotation = worldRot - m_Parent->GetWorldRotation();
            }
            else
            {
                m_LocalRotation = worldRot;
            }
            SetDirty();
        }

        [[nodiscard]] Matrix3 ToMatrix3() const
        {
            Vec3 pos = GetWorldPosition();
            Rotator rot = GetWorldRotation();
            Vec3 scale = GetWorldScale();
            
            return Matrix3::Translation(pos.x, pos.y) * 
                   Matrix3::Rotation(rot.yaw) * 
                   Matrix3::Scale(scale.x, scale.y);
        }

        // --- Hiérarchie ---
        
        void SetParent(Transform* newParent, bool keepWorldTransform = true)
        {
            if (m_Parent == newParent)
                return;

            Vec3 worldPos = GetWorldPosition();
            Rotator worldRot = GetWorldRotation();
            Vec3 worldScale = GetWorldScale();

            if (m_Parent)
            {
                m_Parent->RemoveChild(this);
            }

            m_Parent = newParent;

            if (m_Parent)
            {
                m_Parent->AddChild(this);
            }

            if (keepWorldTransform)
            {
                if (m_Parent)
                {
                    Matrix4 parentInverse = m_Parent->GetWorldMatrix().Inverse();
                    m_LocalPosition = parentInverse.MultiplyPoint(worldPos);

                    m_LocalRotation = worldRot - m_Parent->GetWorldRotation();
                    
                    Vec3 parentScale = m_Parent->GetWorldScale();
                    m_LocalScale.x = (parentScale.x != 0) ? worldScale.x / parentScale.x : 1.0f;
                    m_LocalScale.y = (parentScale.y != 0) ? worldScale.y / parentScale.y : 1.0f;
                    m_LocalScale.z = (parentScale.z != 0) ? worldScale.z / parentScale.z : 1.0f;
                }
                else
                {
                    m_LocalPosition = worldPos;
                    m_LocalRotation = worldRot;
                    m_LocalScale = worldScale;
                }
            }
            
            SetDirty();
        }
        
        const Matrix4& GetLocalMatrix() const
        {
            if (m_IsDirty)
            {
                m_LocalMatrix = Matrix4::TRS(m_LocalPosition, m_LocalRotation, m_LocalScale);
                m_IsDirty = false;
            }
            
            return m_LocalMatrix;
        }

        const Matrix4& GetWorldMatrix() const
        {
            const Matrix4& local = GetLocalMatrix();
            
            if (m_Parent)
            {
                m_WorldMatrix = m_Parent->GetWorldMatrix() * local;
            }
            else
            {
                m_WorldMatrix = local;
            }
            return m_WorldMatrix;
        }

        Vec3 GetWorldPosition() const
        {
            return GetWorldMatrix().GetTranslation(); 
        }

        Rotator GetWorldRotation() const
        {
            if (m_Parent)
                return m_Parent->GetWorldRotation() + m_LocalRotation;
            
            return m_LocalRotation;
        }

        Vec3 GetWorldScale() const
        {
            if (m_Parent)
                return m_Parent->GetWorldScale() * m_LocalScale;
            
            return m_LocalScale;
        }

    private:
        void SetDirty()
        {
            if (m_IsDirty)
                return;
            
            m_IsDirty = true;
            
            for (auto* child : m_Children)
            {
                child->SetDirty();
            }
        }

        void AddChild(Transform* child)
        {
            m_Children.push_back(child);
        }

        void RemoveChild(Transform* child)
        {
            auto it = std::ranges::remove(m_Children, child).begin();
            
            if (it != m_Children.end())
            {
                m_Children.erase(it, m_Children.end());
            }
        }
    };
}
