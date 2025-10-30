#pragma once

#include <vector>

#include "Core/Containers/String.h"
#include "Engine/Render/SpriteAnimator.h"

namespace BixEngine::Render
{
    class Texture;

    /**
     * @brief Helper utilities to generate sprite frames from atlases and external descriptions.
     */
    class SpriteAtlasUtils
    {
    public:
        /** Generates frames by slicing an atlas with a fixed grid. */
        static std::vector<SpriteFrame> LoadFramesFromAtlas(Texture& texture, int columns, int rows, int padding = 0, int margin = 0);

        /** Loads frame definitions from a JSON file compatible with TexturePacker/Godot formats. */
        static std::vector<SpriteFrame> LoadFramesFromJSON(Texture& texture, const std::string& jsonPath);

        /** Loads animations from a .spriteanim asset file. */
        static std::vector<SpriteAnimation> LoadAnimationsFromAsset(const std::string& assetPath, Texture& texture);
    };
}
