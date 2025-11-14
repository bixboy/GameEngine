#include "Ressources/Loaders/SpriteAtlasLoader.h"
#include "Ressources/SpriteAtlas.h"
#include "Logger.h"

namespace BixEngine::resources
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
