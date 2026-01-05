#include "Components/Core/CameraComponent.h"
#include "Framework/Actor.h"
#include "Framework/Scene.h"
#include "Systems/Core/Window.h" 
#include <imgui.h>

namespace BixEngine::Game
{
    CameraComponent* CameraComponent::s_MainCamera = nullptr;

    CameraComponent::CameraComponent() = default;

    CameraComponent::CameraComponent(Actor* owner) : Super(owner)
    {
    }

    CameraComponent::~CameraComponent()
    {
        OnDestroy();
    }

    void CameraComponent::BeginPlay()
    {
        Super::BeginPlay();
        
        if (IsActive)
        {
            SetAsMainCamera();
        }

        if (owner_)
        {
            initialActorPos_ = owner_->GetTransform().GetWorldPosition();
        }
    }

    void CameraComponent::OnDestroy()
    {
        if (s_MainCamera == this)
        {
            s_MainCamera = nullptr;
        }
    }

    void CameraComponent::SetAsMainCamera()
    {
        s_MainCamera = this;
    }

    CameraComponent* CameraComponent::GetMainCamera()
    {
        return s_MainCamera;
    }

    void CameraComponent::LookAt(const Math::Vector3& target)
    {
        if (owner_)
        {
            owner_->GetTransform().LookAt(target);
        }
    }

    Math::Vector2<float> CameraComponent::WorldToScreen(const Math::Vector3& worldPos) const
    {
        float screenWidth = 1600.0f;
        float screenHeight = 900.0f;

        if (owner_)
        {
            Scene* scene = owner_->GetOwningScene();
            if (scene && scene->HasWindow())
            {
               auto& win = scene->GetWindow();
               screenWidth = static_cast<float>(win.GetWidth());
               screenHeight = static_cast<float>(win.GetHeight());
            }
        }
        

        Math::Vector3 currentActorPos = owner_ ? owner_->GetTransform().GetWorldPosition() : Math::Vector3();
        
        float baseX = LockX ? initialActorPos_.x : currentActorPos.x;
        float baseY = LockY ? initialActorPos_.y : currentActorPos.y;

        float camX = baseX + Offset.x;
        float camY = baseY + Offset.y;
        
        float screenX = (worldPos.x - camX) * Zoom + (screenWidth * 0.5f);
        float screenY = (worldPos.y - camY) * Zoom + (screenHeight * 0.5f);

        return { screenX, screenY };
    }

    Math::Vector2<float> CameraComponent::ScreenToWorld(const Math::Vector2<float>& screenPos) const
    {
        float screenWidth = 1600.0f;
        float screenHeight = 900.0f;

        if (owner_)
        {
            Scene* scene = owner_->GetOwningScene();
            if (scene && scene->HasWindow())
            {
               auto& win = scene->GetWindow();
               screenWidth = static_cast<float>(win.GetWidth());
               screenHeight = static_cast<float>(win.GetHeight());
            }
        }

        Math::Vector3 currentActorPos = owner_ ? owner_->GetTransform().GetWorldPosition() : Math::Vector3();

        float baseX = LockX ? initialActorPos_.x : currentActorPos.x;
        float baseY = LockY ? initialActorPos_.y : currentActorPos.y;

        float camX = baseX + Offset.x;
        float camY = baseY + Offset.y;

        
        
        float worldX = (screenPos.x - (screenWidth * 0.5f)) / Zoom + camX;
        float worldY = (screenPos.y - (screenHeight * 0.5f)) / Zoom + camY;

        return { worldX, worldY };
    }

    void CameraComponent::DrawInspectorUI()
    {

    }
}
