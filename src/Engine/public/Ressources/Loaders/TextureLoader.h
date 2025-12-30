#pragma once
#include <memory>
#include "Containers/String.h"

struct SDL_Renderer;

namespace BixEngine::Resources
{
    class Texture;

    std::shared_ptr<Texture> LoadTexture(const String& path, SDL_Renderer* renderer);
}
