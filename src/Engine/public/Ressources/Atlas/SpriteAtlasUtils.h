#pragma once
#include <filesystem>
#include <vector>
#include "Containers/String.h"
#include "Render/Sprite/SpriteFrame.h"


namespace BixEngine::Resources
{
    class Texture;

    struct SpriteAtlasDefinition
    {
        int columns{0};
        int rows{0};
        int padding{0};
        int margin{0};
        String texturePath{};
    };

    struct SpriteAnimationDefinition
    {
        String name{};
        float frameRate{0.0f};
        bool loop{true};
        std::vector<size_t> frames{};
    };

    namespace SpriteAtlasUtils
    {
        bool AutoDetectGrid(const std::filesystem::path& texturePath, int& outCols, int& outRows);
        
        bool ParseAtlasFile(const String& path, SpriteAtlasDefinition& outDefinition, std::vector<SpriteAnimationDefinition>& outAnimations);
        
        std::vector<SpriteFrame> GenerateFrames(Texture& texture, int columns, int rows, int padding, int margin);
    }
}