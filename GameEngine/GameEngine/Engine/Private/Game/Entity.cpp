#include "Game/Entity.h"

#include "Graphics/Renderer.h"

namespace Engine::Game {

Entity::Entity(float x, float y, float w, float h, SDL_Color color)
    : transform_(Math::Vector3(x, y, 0.0f), Math::Rotator(), Math::Vector3(w, h, 1.0f)), color_(color) {}

Entity::Entity(const Math::Transform& transform, SDL_Color color)
    : transform_(transform), color_(color) {}

void Entity::Update(float /*deltaTime*/) {}

void Entity::Render(Graphics::Renderer& renderer) const {
    renderer.SetColor(color_.r, color_.g, color_.b, color_.a);

    SDL_FRect rect{
        transform_.position.x,
        transform_.position.y,
        transform_.scale.x,
        transform_.scale.y,
    };

    SDL_RenderFillRect(renderer.GetSDLRenderer(), &rect);
}

}  // namespace Engine::Game

