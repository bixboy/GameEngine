#pragma once
#include <filesystem>
#include "Engine/Render/Texture.h"

namespace BixEngine::Assets
{
    class TextureLoader
    {
    public:

        static std::shared_ptr<Render::Texture> LoadFromFile(const std::filesystem::path& path, SDL_Renderer* renderer);
    };
}
