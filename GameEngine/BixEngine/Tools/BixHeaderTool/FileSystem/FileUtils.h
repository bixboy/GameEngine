#pragma once
#include <string>
#include <vector>
#include <filesystem>

namespace BixTool
{
    bool WriteFileIfDifferent(const std::filesystem::path& path, const std::string& content);
    
    std::vector<std::filesystem::path> CollectHeaderFiles(const std::vector<std::filesystem::path>& roots);
}
