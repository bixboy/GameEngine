#include "Engine/Assets/TextureLoader.h"
#include "Core/Logger.h"

namespace BixEngine::Assets
{
    std::shared_ptr<Render::Texture> TextureLoader::LoadFromFile(const std::filesystem::path& path, SDL_Renderer* renderer)
    {
        auto texture = std::make_shared<Render::Texture>();

        if (!texture->LoadFromFile(path.string(), renderer))
        {
            LOG_ERROR("Failed to load texture: " + path.string());
            return nullptr;
        }

        return texture;
    }
}
