#pragma once
#include <memory>
#include <type_traits>
#include <vector>
#include <tuple>
#include "Object.h"
#include "Components/Component.h"
#include "Scene.h"
#include "Math/Transform.h"
#include "Actor.generated.h"


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
        virtual void SetupInput(Input::InputManager& inputManager) { (void)inputManager; }

        void AddComponent(std::unique_ptr<Component> component);

        [[nodiscard]] String GetTypeName() const noexcept override { return "Actor"; }

        [[nodiscard]] virtual std::unique_ptr<Actor> ClonePrototype() const;

        [[nodiscard]] const std::vector<std::unique_ptr<Component>>& GetComponents() const noexcept
        {
            return components_;
        }

        [[nodiscard]] std::vector<std::unique_ptr<Component>>& GetComponents() noexcept { return components_; }

        [[nodiscard]] bool IsActive() const noexcept { return active_; }
        void SetActive(bool active) noexcept { active_ = active; }

        bool RemoveComponent(const Component* component);

        void SetOwningScene(Scene* scene) noexcept { owningScene_ = scene; }

        [[nodiscard]] Scene* GetOwningScene() const noexcept { return owningScene_; }

        [[nodiscard]] bool HasBegunPlay() const noexcept { return has_begun_play_; }

    protected:
        virtual void OnComponentRemoved(const Component& /*component*/)
        {
        }

    private:
        std::vector<std::unique_ptr<Component>> components_;
        bool has_begun_play_{false};
        bool active_{true};
        Scene* owningScene_{nullptr};

    public:
        
        template <typename TComponent>
        TComponent* GetComponent() noexcept
        {
            static_assert(std::is_base_of_v<Component, TComponent>, "TComponent must derive from Game::Component");

            for (auto& component : components_)
            {
                if (auto casted = dynamic_cast<TComponent*>(component.get()))
                    return casted;
            }

            return nullptr;
        }

        template <typename TComponent, typename... TArgs>
        TComponent* AddComponent(TArgs&&... args)
        {
            static_assert(std::is_base_of_v<Component, TComponent>, "Component must derive from Game::Component");

            if constexpr (sizeof...(TArgs) == 1)
            {
                using ArgType = std::tuple_element_t<0, std::tuple<TArgs...>>;
                if constexpr (std::is_convertible_v<ArgType, const resources::ComponentPrefab*>)
                {
                    auto component = std::make_unique<TComponent>(this);
                    
                    const resources::ComponentPrefab* prefab = { args... };
                    if (prefab)
                    {
                        component->LoadFromPrefab(prefab);
                    }
                    
                    TComponent* componentRef = component.get();
                    AddComponent(std::move(component));
                    return componentRef;
                }
            }

            auto component = std::make_unique<TComponent>(this, std::forward<TArgs>(args)...);
            TComponent* rawPtr = component.get();
            AddComponent(std::move(component));

            return rawPtr;
        }
    };
}
