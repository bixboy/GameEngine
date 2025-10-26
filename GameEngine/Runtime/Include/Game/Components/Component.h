#pragma once

#include "Core/Containers/String.h"
#include "Reflection/BixReflection.h"
#include "Component.generated.h"

namespace BixEngine::Graphics { class Renderer; }

namespace BixEngine::Game
{
    class Actor;

    BCLASS()
    class Component
    {
        GENERATED_BODY()
        
    public:
        explicit Component(Actor* owner) : owner_(owner) {}
        virtual ~Component() = default;
        
        virtual void BeginPlay() {}
        virtual void Update(float /*deltaTime*/) {}
        virtual void Render(Graphics::Renderer& /*renderer*/) const {}

        [[nodiscard]] virtual String GetTypeName() const { return "Component"; }
        
        virtual void DrawInspectorUI() {}

    protected:
        Actor* owner_{nullptr};
    };
}
