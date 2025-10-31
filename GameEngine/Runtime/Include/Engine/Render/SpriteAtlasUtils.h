#pragma once

#include <string>
#include <vector>

#include "Engine/Render/SpriteAnimator.h"
#include "Engine/Render/SpriteFrame.h"

namespace BixEngine::Ressources
{
    class Texture;

    struct SpriteAtlasDefinition
    {
        int Columns{0};
        int Rows{0};
        int Padding{0};
        int Margin{0};
        std::string TexturePath;
    };

    namespace SpriteAtlasUtils
    {
        std::string LoadFileContents(const std::string& path);
        SpriteAtlasDefinition ParseDefinition(const std::string& contents);

        std::vector<SpriteFrame> LoadFramesFromAtlas(Texture& texture, int columns, int rows, int padding = 0, int margin = 0);
        std::vector<SpriteFrame> LoadFramesFromJSON(Texture& texture, const std::string& jsonPath);

        std::vector<SpriteAnimation> BuildAnimationsFromContent(const std::string& contents, const std::vector<SpriteFrame>& frames);
    }
}

