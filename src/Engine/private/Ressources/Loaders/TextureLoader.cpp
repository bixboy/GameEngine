#include "Ressources/Loaders/TextureLoader.h"
#include "Ressources/RessourcesClass/Texture.h"
#include "Logger.h"

namespace BixEngine::resources
{
    std::shared_ptr<Texture> LoadTexture(const String& path, SDL_Renderer* renderer)
    {
        auto tex = std::make_shared<Texture>();
        if (!tex->LoadFromFile(path, renderer))
        {
            LOG_ERROR("❌ Failed to load texture: " + path);
            return nullptr;
        }
        return tex;
    }
}
