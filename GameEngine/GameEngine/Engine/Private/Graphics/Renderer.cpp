#include "../../Public/Graphics/Renderer.h"
#include <iostream>
#include <ostream>

Renderer::Renderer(SDL_Window* window)
{
    renderer = SDL_CreateRenderer(window, nullptr);
    
    if (!renderer)
        std::cerr << "Renderer creation failed: " << SDL_GetError() << '\n';
}

Renderer::~Renderer()
{
    if (renderer)
    {
        SDL_DestroyRenderer(renderer);
        renderer = nullptr;
    }
}

void Renderer::Clear()
{
    if (renderer)
        SDL_RenderClear(renderer);
}

void Renderer::Present()
{
    if (renderer)
        SDL_RenderPresent(renderer);
}

void Renderer::SetColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    if (renderer)
        SDL_SetRenderDrawColor(renderer, r, g, b, a);
}