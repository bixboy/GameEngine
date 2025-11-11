#pragma once
#include <filesystem>

namespace BixEngine::HeaderGeneratorUtils
{
    void RunBixHeaderTool(const std::filesystem::path& toolPath, const std::filesystem::path& headerPath);
}

