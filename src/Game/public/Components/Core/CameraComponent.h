#pragma once
#include "Components/Core/Component.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/Color.h"
#include "CameraComponent.generated.h"


namespace BixEngine::Game
{
    BCLASS()
    class CameraComponent : public Component
    {
        GENERATED_BODY()

    public:
        using Super = Component;

        CameraComponent();
        explicit CameraComponent(Actor* owner);
        ~CameraComponent() override;

        void BeginPlay() override;
        void OnDestroy();
        
        // --- Gestion Caméra Principale ---
        void SetAsMainCamera();
        static CameraComponent* GetMainCamera();

        // --- Utilitaires ---
        void SetAspectRatio(float ratio);
        [[nodiscard]] float GetAspectRatio() const { return aspectRatio_; }

        [[nodiscard]] Math::Vector2 WorldToScreen(const Math::Vector3& worldPos) const;
        [[nodiscard]] Math::Vector2 ScreenToWorld(const Math::Vector2& screenPos) const;

        // --- Matrices ---
        
        [[nodiscard]] Math::Matrix4 GetViewMatrix() const;
        [[nodiscard]] Math::Matrix4 GetProjectionMatrix() const;

        // --- Propriétés ---

        BPROPERTY(Category="Camera|Settings")
        float Zoom{1.0f};

        BPROPERTY(Category="Camera|Settings")
        Math::Vector2 Offset{0.0f, 0.0f};

        BPROPERTY(Category="Camera|Settings")
        Math::Color ClearColor{0.0f, 0.0f, 0.0f, 1.0f};

        BPROPERTY(Category="Camera|Settings")
        bool StartAsMainCamera{true};

        // --- Contraintes ---

        BPROPERTY(Category="Camera|Constraints")
        bool LockX{false};

        BPROPERTY(Category="Camera|Constraints")
        bool LockY{false};

    private:
        Math::Vector2 GetViewportSize() const;

        static CameraComponent* s_MainCamera;
        Math::Vector3 initialPosition_{0.0f, 0.0f, 0.0f};
        float aspectRatio_{16.0f / 9.0f};
    };
}