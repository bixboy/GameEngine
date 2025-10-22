#pragma once

#include "Bix/Core/String.h"
#include "Bix/Game/Object.h"
#include "Bix/Game/Scripting/ScriptReflection.h"

namespace BixEngine::Graphics { class Renderer; }

namespace BixEngine::Game
{
    class Actor;

    BCLASS(Component, Scripting::ScriptBase, Object)
    class Component : public Object, public Scripting::ScriptBase
    {
    public:
        BIX_GENERATED_BODY();

        Component() : Object("Component") {}
        explicit Component(Actor* owner) : Object("Component"), owner_(owner) {}
        virtual ~Component() = default;

        virtual void BeginPlay() {}
        virtual void Update(float /*deltaTime*/) {}
        virtual void Render(Graphics::Renderer& /*renderer*/) const {}

        [[nodiscard]] String GetTypeName() const noexcept override { return "Component"; }

        /// Draws the component specific inspector user interface. Override in
        /// derived components to expose editable properties inside the actor
        /// inspector. The default implementation intentionally does nothing so
        /// that callers can detect the absence of custom UI.
        virtual void DrawInspectorUI() {}

        void SetOwner(Actor* owner) noexcept { owner_ = owner; }
        [[nodiscard]] Actor* GetOwner() const noexcept { return owner_; }

    protected:
        static void RegisterProperties(::BixEngine::Engine::SaveSystem::BixClass& cls);

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

