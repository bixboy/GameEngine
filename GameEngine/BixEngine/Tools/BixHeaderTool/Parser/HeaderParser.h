#pragma once
#include <filesystem>
#include "Types.h"

namespace BixTool
{
    HeaderParseResult ParseHeader(const std::filesystem::path& filePath);

    std::string MakeQualifiedName(const std::vector<std::string>& namespaces, const std::string& name);
    std::string MakeScopedName(const std::vector<std::string>& namespaces, const std::string& name);
    std::string SanitizeBaseType(const std::string& base);
}