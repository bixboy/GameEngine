#include "Ressources/Loaders/ResourceLoaders.h"
#include "Ressources/Core/ResourceManager.h"
#include "Ressources/RessourcesClass/Texture.h"
#include "Ressources/RessourcesClass/SpriteAtlas.h"
#include "Debug/Logger.h"
#include "Ressources/Loaders/SpriteAtlasLoader.h"
#include "Ressources/Loaders/TextureLoader.h"
#include "Ressources/RessourcesClass/AudioClip.h"
#include "Ressources/RessourcesClass/ComponentPrefab.h"
#include "Ressources/RessourcesClass/AudioContainer.h"

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

        LOG_INFO("Registering AudioClip loader...");
        rm.RegisterLoader<AudioClip>(
            [](const String& path)
            {
                auto clip = std::make_shared<AudioClip>();
                if (clip->LoadFromFile(path))
                    return clip;
                return std::shared_ptr<AudioClip>(nullptr);
            });

        LOG_INFO("Registering ComponentPrefab loader...");
        rm.RegisterLoader<ComponentPrefab>(
            [](const String& path)
            {
                auto prefab = std::make_shared<ComponentPrefab>();
                if (prefab->LoadFromFile(path))
                    return prefab;
                return std::shared_ptr<ComponentPrefab>(nullptr);
            });

        LOG_INFO("Registering AudioContainer loader...");
        rm.RegisterLoader<AudioContainer>(
            [](const String& path)
            {
                auto container = std::make_shared<AudioContainer>();
                if (container->LoadFromFile(path))
                    return container;
                return std::shared_ptr<AudioContainer>(nullptr);
            });

        LOG_INFO("All resource loaders registered successfully.");
    }
}
