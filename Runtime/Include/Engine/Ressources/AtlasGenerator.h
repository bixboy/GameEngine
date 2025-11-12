#pragma once

#include <filesystem>

#include "Core/Containers/String.h"

namespace BixEngine::resources
{
    /**
     * @brief Utility class capable of generating sprite atlases and .atlas metadata files from folders of PNG frames.
     */
    class AtlasGenerator
    {
    public:
        static bool GenerateAtlas(const std::filesystem::path& frameDirectory,
                                  int columns,
                                  int rows,
                                  int padding,
                                  int margin,
                                  float frameRate,
                                  bool loop);
    };
}
