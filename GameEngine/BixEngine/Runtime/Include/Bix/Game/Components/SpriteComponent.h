#pragma once

#include "Bix/Game/Components/Component.h"
#include "Bix/Reflection/BixReflection.h"
#include "SDL3/SDL.h"
#include "SpriteComponent.generated.h"

namespace BixEngine::Game
{
    BCLASS()
    class SpriteComponent : public Component
    {
        GENERATED_BODY()
        
    public:

        explicit SpriteComponent(Actor* owner);
        SpriteComponent(Actor* owner, SDL_Color color, float w, float h) : Component(owner), color_(color), width_(w), height_(h) {}

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

        [[nodiscard]] String GetTypeName() const override { return "SpriteComponent"; }

    private:
        BPROPERTY()
        SDL_Color color_;

        BPROPERTY()
        float width_;

        BPROPERTY()
        float height_;
    };
}
