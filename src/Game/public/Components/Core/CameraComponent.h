#pragma once
#include "Components/Core/Component.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include <SDL3/SDL_pixels.h>
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
        void DrawInspectorUI() override;

        [[nodiscard]] String GetTypeName() const override { return "CameraComponent"; }

        // ────────────────────────────────────────────────
        // Public API
        // ────────────────────────────────────────────────
        
        void SetAsMainCamera();

        [[nodiscard]] Math::Vector2<float> WorldToScreen(const Math::Vector3& worldPos) const;
        [[nodiscard]] Math::Vector2<float> ScreenToWorld(const Math::Vector2<float>& screenPos) const;

        static CameraComponent* GetMainCamera();

        // ────────────────────────────────────────────────
        // Properties
        // ────────────────────────────────────────────────
        
        BPROPERTY()
        float Zoom{1.0f};

        BPROPERTY()
        Math::Vector2<float> Offset{0.0f, 0.0f};

        BPROPERTY()
        SDL_Color ClearColor{0, 0, 0, 255}; // Default Black

        BPROPERTY()
        bool IsActive{false};

        BPROPERTY(Category="Constraints")
        bool LockX{false};

        BPROPERTY(Category="Constraints")
        bool LockY{false};

    private:
        static CameraComponent* s_MainCamera;
        
        Math::Vector3 initialActorPos_{0.0f, 0.0f, 0.0f};

    };
}
