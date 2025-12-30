#pragma once
#include <filesystem>


namespace BixEngine::Resources
{
    class AtlasGenerator
    {
    public:

        [[nodiscard]] static bool GenerateAtlas(const std::filesystem::path& frameDirectory, int columns = 0, int rows = 0, int padding = 2, int margin = 2,
            float frameRate = 12.0f, bool loop = true);
    };
}