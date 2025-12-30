#include "Ressources/Loaders/ResourceLoaders.h"
#include "Ressources/Core/ResourceManager.h"
#include "Debug/Logger.h"

#include "Ressources/Loaders/TextureLoader.h"
#include "Ressources/Loaders/SpriteAtlasLoader.h"

#include "Ressources/RessourcesClass/Texture.h"
#include "Ressources/RessourcesClass/SpriteAtlas.h"
#include "Ressources/RessourcesClass/AudioClip.h"
#include "Ressources/RessourcesClass/AudioContainer.h"


namespace BixEngine::Resources
{
    void RegisterAllResourceLoaders(SDL_Renderer* renderer)
    {
        auto& rm = ResourceManager::Get();

        LOG_INFO("--- Registering Resource Loaders ---");

        // --- Texture ---
        rm.RegisterLoader<Texture>(
            [renderer](const String& path)
            {
                return LoadTexture(path, renderer);
            });

        // --- SpriteAtlas ---
        rm.RegisterLoader<SpriteAtlas>(
            [](const String& path)
            {
                return LoadSpriteAtlas(path);
            });

        // --- AudioClip ---
        rm.RegisterLoader<AudioClip>(
            [](const String& path)
            {
                auto clip = std::make_shared<AudioClip>();
                if (clip->LoadFromFile(path))
                    return clip;
                
                LOG_ERROR("❌ Failed to load AudioClip: " + path);
                return std::shared_ptr<AudioClip>(nullptr);
            });

        // --- AudioContainer ---
        rm.RegisterLoader<AudioContainer>(
            [](const String& path)
            {
                auto container = std::make_shared<AudioContainer>();
                if (container->LoadFromFile(path))
                    return container;

                LOG_ERROR("❌ Failed to load AudioContainer: " + path);
                return std::shared_ptr<AudioContainer>(nullptr);
            });

        LOG_INFO("All resource loaders registered successfully.");
    }
}