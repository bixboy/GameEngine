#pragma once

#include "Bix/Core/String.h"
#include "Bix/Game/Scripting/ScriptReflection.h"

namespace BixEngine::Graphics { class Renderer; }

namespace BixEngine::Game
{
    class Actor;

    class Component : public Scripting::ScriptBase
    {
    public:
        BIX_GENERATED_BODY(Component);
        BIX_DECLARE_SCRIPT_CLASS(Component, Scripting::ScriptBase);

        explicit Component(Actor* owner) : owner_(owner) {}
        virtual ~Component() = default;
        
        virtual void BeginPlay() {}
        virtual void Update(float /*deltaTime*/) {}
        virtual void Render(Graphics::Renderer& /*renderer*/) const {}

        [[nodiscard]] virtual String GetTypeName() const { return "Component"; }

        /// Draws the component specific inspector user interface. Override in
        /// derived components to expose editable properties inside the actor
        /// inspector. The default implementation intentionally does nothing so
        /// that callers can detect the absence of custom UI.
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
