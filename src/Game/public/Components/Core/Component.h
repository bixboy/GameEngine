#pragma once
#include "Utils/ReflectionMacros.h"
#include "Containers/String.h"
#include "Renderer.h"
#include "Component.generated.h"
#include "Component.generated.h"
#include "Math/Transform.h"

namespace BixEngine::Game { class Actor; }


namespace BixEngine::Game
{
    BCLASS()
    class Component
    {
        GENERATED_BODY()

    public:
        Component() = default;
        explicit Component(Actor* owner) : owner_(owner) {}

        virtual ~Component() = default;

        virtual void BeginPlay() {}
        virtual void Update(float deltaTime) { (void)deltaTime; }
        virtual void Render(Graphics::Renderer& renderer) const { (void)renderer; }
        
        virtual void DrawInspectorUI() {}

        [[nodiscard]] virtual String GetTypeName() const { return "Component"; }

        void SetOwner(Actor* owner) { owner_ = owner; }
        [[nodiscard]] Actor* GetOwner() const { return owner_; }

        [[nodiscard]] bool IsActive() const { return active_; }
        void SetActive(bool active) { active_ = active; }

        [[nodiscard]] Math::Transform& GetTransform();
        [[nodiscard]] const Math::Transform& GetTransform() const;

    protected:
        Actor* owner_{nullptr};
        bool active_{true};
    };
}