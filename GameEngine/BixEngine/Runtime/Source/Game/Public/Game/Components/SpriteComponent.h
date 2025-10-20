#pragma once

#include "Game/Components/Component.h"
#include "SDL3/SDL.h"

namespace BixEngine::Game
{
    class SpriteComponent : public Component
    {
    public:
        SpriteComponent(Actor* owner, SDL_Color color, float w, float h)
            : Component(owner), color_(color), width_(w), height_(h) {}

        void Render(Graphics::Renderer& renderer) const override;

        void SetColor(SDL_Color color) noexcept { color_ = color; }
        void SetDimensions(float w, float h) noexcept
        {
            width_ = w;
            height_ = h;
        }

        [[nodiscard]] SDL_Color GetColor() const noexcept { return color_; }
        [[nodiscard]] float GetWidth() const noexcept { return width_; }
        [[nodiscard]] float GetHeight() const noexcept { return height_; }

        [[nodiscard]] BixEngine::String GetTypeName() const override { return "SpriteComponent"; }

    private:
        SDL_Color color_;
        float width_;
        float height_;
    };
}
