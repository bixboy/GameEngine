#include "Ressources/Loaders/SpriteAtlasLoader.h"
#include "Ressources/RessourcesClass/SpriteAtlas.h"
#include "Debug/Logger.h"

namespace BixEngine::Resources
{
    std::shared_ptr<SpriteAtlas> LoadSpriteAtlas(const String& path)
    {
        auto atlas = std::make_shared<SpriteAtlas>();
        
        if (!atlas->LoadFromFile(path))
        {
            LOG_ERROR("❌ Failed to load sprite atlas: " + path);
            return nullptr;
        }
        
        return atlas;
    }
}
