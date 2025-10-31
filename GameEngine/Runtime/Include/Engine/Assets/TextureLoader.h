#pragma once
#include <filesystem>
#include "Engine/Ressources/Texture.h"

namespace BixEngine::Assets
{
    class TextureLoader
    {
    public:

        static std::shared_ptr<Ressources::Texture> LoadFromFile(const std::filesystem::path& path, SDL_Renderer* renderer);
    };
}
