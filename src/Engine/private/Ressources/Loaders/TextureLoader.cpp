#include "Ressources/Loaders/TextureLoader.h"
#include "Ressources/RessourcesClass/Texture.h"
#include "Debug/Logger.h"

namespace BixEngine::Resources
{
    std::shared_ptr<Texture> LoadTexture(const String& path, SDL_Renderer* renderer)
    {
        auto tex = std::make_shared<Texture>();

        if (!tex->LoadFromFile(path, renderer))
        {
            LOG_ERROR("Failed to load texture: " + path);
            return nullptr;
        }
        
        return tex;
    }
}