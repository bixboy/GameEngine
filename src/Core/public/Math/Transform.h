#pragma once
#include "Math/Vector3.h"
#include "Math/Rotator.h"
#include "Matrix/Matrix4.h"
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
