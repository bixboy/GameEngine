#pragma once

#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "Core/Containers/String.h"
#include "Engine/Render/SpriteFrame.h"

struct SDL_Renderer;

namespace BixEngine::Ressources
{
    class Texture;

    /**
     * @brief Centralized cache responsible for loading textures and sharing atlas data.
     */
    class TextureManager
    {
    public:
        static TextureManager& Get();

        /** Loads or retrieves a cached texture from disk. */
        std::shared_ptr<Texture> LoadTexture(const String& path, SDL_Renderer* renderer);

        /** Stores generated frames for reuse keyed by atlas identifier. */
        void CacheAtlas(const String& atlasId, std::vector<SpriteFrame> frames);

        /** Returns cached frames for an atlas if it exists. */
        [[nodiscard]] std::vector<SpriteFrame> GetCachedAtlas(const String& atlasId) const;

    private:
        TextureManager() = default;

        mutable std::mutex mutex_;
        std::unordered_map<String, std::weak_ptr<Texture>> textures_;
        std::unordered_map<String, std::vector<SpriteFrame>> atlasCache_;
    };
}
