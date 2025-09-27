#pragma once
#include "Game/Components/Component.h"
#include "SDL3/SDL.h"

namespace Engine::Game
{
    class SpriteComponent : public Component
    {
        public:
            SpriteComponent(Actor* owner, SDL_Color color, float w, float h)
                : Component(owner), color_(color), width_(w), height_(h) {}

            void Render(Graphics::Renderer& renderer) const override;

        private:
            SDL_Color color_;
            float width_;
            float height_;
    };
}