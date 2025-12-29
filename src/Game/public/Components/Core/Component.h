#pragma once
#include "Utils/ReflectionMacros.h"
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

        Component() = default;
        explicit Component(Actor* owner) : owner_(owner) {}

        virtual ~Component() = default;

        virtual void BeginPlay(){}
        virtual void Update(float  ){}
        virtual void Render(Graphics::Renderer&  ) const{}

        [[nodiscard]] virtual String GetTypeName() const { return "Component"; }

        virtual void DrawInspectorUI(){}
        
        virtual void LoadFromPrefab(const resources::ComponentPrefab*  ) {}

        void SetOwner(Actor* owner) { owner_ = owner; }
        [[nodiscard]] Actor* GetOwner() const { return owner_; }

    protected:
        Actor* owner_{nullptr};
    };
}
