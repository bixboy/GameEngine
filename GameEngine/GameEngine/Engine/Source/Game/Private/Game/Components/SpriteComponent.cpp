#include "Game/Components/SpriteComponent.h"
#include "Game/Actor.h"
#include "Graphics/Renderer.h"

namespace Engine::Game
{
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