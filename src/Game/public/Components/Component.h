#pragma once
#include "ReflectionMacros.h"
#include "Containers/String.h"
#include "Renderer.h"
#include "Component.generated.h"

namespace BixEngine::Game
{
    class Actor;
}

namespace BixEngine::resources
{
    class ComponentPrefab;
}

namespace BixEngine::Game
{

    BCLASS()

    class Component
    {
        GENERATED_BODY()

    public:
        using Super = Component;

        explicit Component(Actor* owner) : owner_(owner) {}

        virtual ~Component() = default;

        virtual void BeginPlay(){}
        virtual void Update(float /*deltaTime*/){}
        virtual void Render(Graphics::Renderer& /*renderer*/) const{}

        [[nodiscard]] virtual String GetTypeName() const { return "Component"; }

        virtual void DrawInspectorUI(){}
        
        virtual void LoadFromPrefab(const resources::ComponentPrefab* /*prefab*/) {}

    protected:
        Actor* owner_{nullptr};
    };
}
