#pragma once
#include <memory>
#include <vector>
#include "Game/Object.h"
#include "Math/Math.h"

namespace Engine::Graphics { class Renderer; }

namespace Engine::Game
{
    class Component;
    
    class Actor : public Object
    {
        public:
            explicit Actor(const Math::Transform& transform = Math::Transform());
        
            Actor(String name, const Math::Transform& transform = Math::Transform());
        
            virtual void BeginPlay();
            virtual void Update(float deltaTime);
            virtual void Render(Graphics::Renderer& renderer) const;

             void AddComponent(std::unique_ptr<Component> component);

            [[nodiscard]] String GetTypeName() const noexcept override { return "Actor"; }

            [[nodiscard]] virtual std::unique_ptr<Actor> ClonePrototype() const;

        public:
            [[nodiscard]] const std::vector<std::unique_ptr<Component>>& GetComponents() const noexcept { return components_; }

        private:
        std::vector<std::unique_ptr<Component>> components_;

        bool has_begun_play_{false};
    };
}
