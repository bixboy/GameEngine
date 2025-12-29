#pragma once
#include <filesystem>


namespace BixEngine::resources
{
     
    class AtlasGenerator
    {
    public:
        static bool GenerateAtlas(const std::filesystem::path& frameDirectory, int columns, int rows, int padding, int margin, float frameRate, bool loop);
    };
}
