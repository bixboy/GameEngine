#include "Components/Core/CameraComponent.h"
#include "Framework/Actor.h"
#include "Framework/Scene.h"
#include "Systems/Core/Window.h"


namespace BixEngine::Game
{
    CameraComponent* CameraComponent::s_MainCamera = nullptr;

    CameraComponent::CameraComponent() = default;
    CameraComponent::CameraComponent(Actor* owner) : Super(owner) {}

    CameraComponent::~CameraComponent()
    {
        OnDestroy();
    }

    void CameraComponent::OnDestroy()
    {
        if (s_MainCamera == this)
            s_MainCamera = nullptr;
    }

    void CameraComponent::BeginPlay()
    {
        Super::BeginPlay();
        
        if (StartAsMainCamera)
        {
            SetAsMainCamera();
        }

        if (owner_)
        {
            initialPosition_ = owner_->GetTransform().GetWorldPosition();
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

    void CameraComponent::SetAspectRatio(float ratio)
    {
        aspectRatio_ = ratio;
    }

    Math::Vector2 CameraComponent::GetViewportSize() const
    {
        if (owner_)
        {
            Scene* scene = owner_->GetOwningScene();
            if (scene && scene->HasWindow())
            {
               auto& win = scene->GetWindow();
               return { static_cast<float>(win.GetWidth()), static_cast<float>(win.GetHeight()) };
            }
        }
        return { 1600.0f, 900.0f };
    }

    Math::Vector2 CameraComponent::WorldToScreen(const Math::Vector3& worldPos) const
    {
        Math::Vector2 screenSize = GetViewportSize();
        Math::Vector3 camPos = owner_ ? owner_->GetTransform().GetWorldPosition() : Math::Vector3();

        float effectiveCamX = LockX ? initialPosition_.x : camPos.x;
        float effectiveCamY = LockY ? initialPosition_.y : camPos.y;

        effectiveCamX += Offset.x;
        effectiveCamY += Offset.y;
        
        float screenX = (worldPos.x - effectiveCamX) * Zoom + (screenSize.x * 0.5f);
        float screenY = (worldPos.y - effectiveCamY) * Zoom + (screenSize.y * 0.5f);

        return { screenX, screenY };
    }

    Math::Vector2 CameraComponent::ScreenToWorld(const Math::Vector2& screenPos) const
    {
        Math::Vector2 screenSize = GetViewportSize();
        Math::Vector3 camPos = owner_ ? owner_->GetTransform().GetWorldPosition() : Math::Vector3();

        float effectiveCamX = LockX ? initialPosition_.x : camPos.x;
        float effectiveCamY = LockY ? initialPosition_.y : camPos.y;

        effectiveCamX += Offset.x;
        effectiveCamY += Offset.y;

        float worldX = (screenPos.x - (screenSize.x * 0.5f)) / Zoom + effectiveCamX;
        float worldY = (screenPos.y - (screenSize.y * 0.5f)) / Zoom + effectiveCamY;

        return { worldX, worldY };
    }

    // --- Matrices ---
    
    Math::Matrix4 CameraComponent::GetProjectionMatrix() const
    {
        Math::Vector2 size = GetViewportSize();

        float halfW = (size.x * 0.5f) / Zoom;
        float halfH = (size.y * 0.5f) / Zoom;
        
        return Math::Matrix4::Orthographic(-halfW, halfW, -halfH, halfH, -1.0f, 1000.0f);
    }

    Math::Matrix4 CameraComponent::GetViewMatrix() const
    {
        if (!owner_)
            return Math::Matrix4::Identity();

        Math::Vector3 camPos = owner_->GetTransform().GetWorldPosition();
        float x = LockX ? initialPosition_.x : camPos.x;
        float y = LockY ? initialPosition_.y : camPos.y;
        
        return Math::Matrix4::Translation({ -(x + Offset.x), -(y + Offset.y), 0.0f });
    }
}