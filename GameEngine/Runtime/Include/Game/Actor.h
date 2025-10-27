#pragma once
#include <memory>
#include <type_traits>
#include <vector>
#include "Game/Object.h"
#include "Core/Math/Math.h"
#include "Actor.generated.h"

namespace BixEngine::Game { class Scene; }
namespace BixEngine::Graphics { class Renderer; }

namespace BixEngine::Game
{
    class Component;

    BCLASS()
    class Actor : public Object
    {
        GENERATED_BODY()
        
        public:
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

            void SetOwningScene(Scene* scene) noexcept { owningScene_ = scene; }
            [[nodiscard]] Scene* GetOwningScene() const noexcept { return owningScene_; }

        protected:
            virtual void OnComponentRemoved(const Component& /*component*/) {}

        private:
            std::vector<std::unique_ptr<Component>> components_;
            bool has_begun_play_{false};
            bool active_{true};
            Scene* owningScene_{nullptr};
    };
}
