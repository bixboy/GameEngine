#pragma once

#include <vector>

#include "Core/Containers/String.h"
#include "Engine/Render/SpriteFrame.h"

namespace BixEngine::resources
{
    class Texture;

    /**
     * @brief Describes the static layout of an atlas.
     */
    struct SpriteAtlasDefinition
    {
        int columns{0};
        int rows{0};
        int padding{0};
        int margin{0};
        String texturePath{};
    };

    /**
     * @brief Describes a single animation entry from a sprite atlas document.
     */
    struct SpriteAnimationDefinition
    {
        String name{};
        float frameRate{0.0f};
        bool loop{true};
        std::vector<size_t> frames{};
    };

    namespace SpriteAtlasUtils
    {
        /**
         * @brief Loads and parses a JSON atlas description.
         * @return True if parsing succeeded.
         */
        bool ParseAtlasFile(const String& path, SpriteAtlasDefinition& outDefinition, std::vector<SpriteAnimationDefinition>& outAnimations);

        /**
         * @brief Generates sprite frames using a grid layout definition.
         */
        std::vector<SpriteFrame> GenerateFrames(Texture& texture, int columns, int rows, int padding, int margin);
    }
}
