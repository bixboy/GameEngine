#include "../../Public/Game/Entity.h"

Entity::Entity(float x, float y, float w, float h, SDL_Color color)
    : x(x), y(y), w(w), h(h), color(color) {}

void Entity::Update(float dt) {
    
}

void Entity::Render(SDL_Renderer* renderer) {
    
    SDL_FRect rect{x, y, w, h};
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &rect);
}

