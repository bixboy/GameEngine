#include "Engine/Render/TextureManager.h"
#include "Engine/Render/Texture.h"
#include "Engine/Render/SpriteFramePool.h"
#include "SDL3/SDL_render.h"

namespace BixEngine::Render
{
    TextureManager& TextureManager::Get()
    {
        static TextureManager instance;
        return instance;
    }

    std::shared_ptr<Texture> TextureManager::LoadTexture(const String& path, SDL_Renderer* renderer)
    {
        std::scoped_lock lock(mutex_);
        auto found = textures_.find(path);
        if (found != textures_.end())
        {
            if (auto shared = found->second.lock())
            {
                return shared;
            }
        }

        auto texture = std::make_shared<Texture>();
        if (!texture->LoadFromFile(path, renderer))
        {
            return nullptr;
        }

        textures_[path] = texture;
        return texture;
    }

    void TextureManager::CacheAtlas(const String& atlasId, std::vector<SpriteFrame> frames)
    {
        std::scoped_lock lock(mutex_);
        atlasCache_[atlasId] = std::move(frames);
    }

    std::vector<SpriteFrame> TextureManager::GetCachedAtlas(const String& atlasId) const
    {
        std::scoped_lock lock(mutex_);
        const auto found = atlasCache_.find(atlasId);
        if (found != atlasCache_.end())
        {
            return found->second;
        }

        return {};
    }
}
