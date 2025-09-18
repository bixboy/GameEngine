#pragma once

namespace Engine::Graphics { class Renderer; }

namespace Engine::Game
{
    class Actor;

    class Component
    {
        public:
            explicit Component(Actor* owner) : owner_(owner) {}
            virtual ~Component() = default;

            virtual void Update(float deltaTime) {}
            virtual void Render(Graphics::Renderer& renderer) const {}

        protected:
            Actor* owner_{nullptr};
    };
}