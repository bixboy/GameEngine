#include "../../Public/Game/Entity.h"
#include "../../Public/Graphics/Renderer.h"

Entity::Entity(float x, float y, float w, float h, SDL_Color color)
    : transform(Vector3(x, y, 0.0f), Rotator(), Vector3(w, h, 1.0f)), color(color) {}

Entity::Entity(const Transform& transform, SDL_Color color)
    : transform(transform), color(color) {}

void Entity::Update(float dt)
{
    
}

void Entity::Render(Renderer& renderer)
{
    renderer.SetColor(color.r, color.g, color.b, color.a);

    SDL_FRect rect{
        transform.position.x,
        transform.position.y,
        transform.scale.x,
        transform.scale.y
    };

    SDL_RenderFillRect(renderer.GetSDLRenderer(), &rect);
}

