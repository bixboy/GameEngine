#pragma once
#include <memory>
#include <type_traits>
#include <vector>
#include "Bix/Game/Object.h"
#include "Bix/Game/Scripting/ScriptReflection.h"
#include "Bix/Math/Math.h"

namespace BixEngine::Graphics { class Renderer; }

namespace BixEngine::Game
{
    class Component;

    class Actor : public Object, public Scripting::ScriptBase
    {
        public:
            BIX_GENERATED_BODY(Actor);
            BIX_DECLARE_SCRIPT_CLASS(Actor, Scripting::ScriptBase);
            BIX_CLASS(Actor, Object);

            explicit Actor(const Math::Transform& transform = Math::Transform());

            Actor(String name, const Math::Transform& transform = Math::Transform());

            virtual void BeginPlay();
            virtual void Update(float deltaTime);
            virtual void Render(Graphics::Renderer& renderer) const;

            void AddComponent(std::unique_ptr<Component> component);

            template<typename TComponent, typename... TArgs>
            TComponent& AddComponent(TArgs&&... args)
            {
                static_assert(std::is_base_of_v<Component, TComponent>, "Component must derive from Game::Component");
                auto component = std::make_unique<TComponent>(this, std::forward<TArgs>(args)...);
                TComponent& componentRef = *component;
                AddComponent(std::move(component));
                return componentRef;
            }

            [[nodiscard]] String GetTypeName() const noexcept override { return "Actor"; }

            [[nodiscard]] virtual std::unique_ptr<Actor> ClonePrototype() const;

        public:
            [[nodiscard]] const std::vector<std::unique_ptr<Component>>& GetComponents() const noexcept { return components_; }
            [[nodiscard]] std::vector<std::unique_ptr<Component>>& GetComponents() noexcept { return components_; }

            [[nodiscard]] bool IsActive() const noexcept { return active_; }
            void SetActive(bool active) noexcept { active_ = active; }

            bool RemoveComponent(const Component* component);

        protected:
            void OnPostDeserialize() override;
            virtual void OnComponentRemoved(const Component& /*component*/) {}

        private:
            BIX_PROPERTY(std::vector<std::unique_ptr<Component>>, components_);

            bool has_begun_play_{false};
            BIX_PROPERTY(bool, active_, = true);
    };
}

