#include "Ressources/Loaders/ResourceLoaders.h"
#include "Ressources/ResourceManager.h"
#include "Ressources/Texture.h"
#include "Ressources/SpriteAtlas.h"
#include "Logger.h"
#include "Ressources/Loaders/SpriteAtlasLoader.h"
#include "Ressources/Loaders/TextureLoader.h"

namespace BixEngine::resources
{
    void RegisterAllResourceLoaders(SDL_Renderer* renderer)
    {
        auto& rm = ResourceManager::Get();

        LOG_INFO("Registering Texture loader...");
        rm.RegisterLoader<Texture>(
            [renderer](const String& path)
            {
                return LoadTexture(path, renderer);
            });

        LOG_INFO("Registering SpriteAtlas loader...");
        rm.RegisterLoader<SpriteAtlas>(
            [](const String& path)
            {
                return LoadSpriteAtlas(path);
            });

        LOG_INFO("All resource loaders registered successfully.");
    }
}
