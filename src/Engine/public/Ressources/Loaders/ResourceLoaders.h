#pragma once

struct SDL_Renderer;

namespace BixEngine::Resources
{
    void RegisterAllResourceLoaders(SDL_Renderer* renderer);
}
