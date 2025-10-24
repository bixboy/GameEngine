#include "Bix/Game/Components/SpriteComponent.h"
#include "Bix/Game/Actor.h"
#include "Bix/Game/Components/ComponentRegistry.h"
#include "Bix/Graphics/Renderer.h"

namespace BixEngine::Game
{
    namespace
    {
        constexpr SDL_Color kDefaultSpriteColor{255, 255, 255, 255};
        constexpr float kDefaultSpriteWidth = 32.0f;
        constexpr float kDefaultSpriteHeight = 32.0f;
    }

    SpriteComponent::SpriteComponent(Actor* owner) : Component(owner), color_(kDefaultSpriteColor), width_(kDefaultSpriteWidth), height_(kDefaultSpriteHeight)
    {
    }

    void SpriteComponent::Render(Graphics::Renderer& renderer) const
    {
        auto pos = owner_->GetPosition();

        SDL_FRect rect
        {
            pos.x,
            pos.y,
            width_,
            height_
        };

        renderer.SetColor(color_.r, color_.g, color_.b, color_.a);
        SDL_RenderFillRect(renderer.GetSDLRenderer(), &rect);
    }
}