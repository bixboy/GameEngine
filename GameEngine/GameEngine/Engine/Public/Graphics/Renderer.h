#pragma once
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>

class Renderer
{
public:
    Renderer(SDL_Window* window);
    ~Renderer();

    void Clear();
    void Present();
    void SetColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a);

    SDL_Renderer* GetSDLRenderer() const { return renderer; }

private:
    SDL_Renderer* renderer = nullptr;
};
