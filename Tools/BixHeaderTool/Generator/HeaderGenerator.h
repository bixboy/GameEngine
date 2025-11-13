#pragma once
#include "../Types.h"
#include <string>
#include <filesystem>

namespace BixTool
{
    std::string GenerateHeader(const std::filesystem::path& headerPath, const HeaderParseResult& result);
}
