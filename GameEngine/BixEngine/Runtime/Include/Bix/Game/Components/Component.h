#pragma once

#include "Bix/Core/String.h"
#include "Bix/Reflection/BixReflection.h"
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

#define BIX_AUTO_REGISTER_COMPONENT(ClassType) \
    namespace { \
        struct BixAutoRegister_##ClassType { \
            BixAutoRegister_##ClassType() { \
                ::BixEngine::Game::ComponentRegistry::GetInstance().RegisterComponent( \
                    #ClassType, \
                    [](::BixEngine::Game::Actor& actor) { actor.AddComponent<ClassType>(); }); \
            } \
        }; \
        static BixAutoRegister_##ClassType s_AutoReg_##ClassType; \
    }
