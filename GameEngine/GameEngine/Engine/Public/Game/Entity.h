#pragma once
#include "SDL3/SDL_pixels.h"
#include "SDL3/SDL_render.h"

class Entity
{
public:
    float x, y, w, h;
    SDL_Color color;

    Entity(float x, float y, float w, float h, SDL_Color color);

    void Update(float dt);
    void Render(SDL_Renderer* renderer);
    
};
