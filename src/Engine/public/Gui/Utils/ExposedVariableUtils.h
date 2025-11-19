#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace BixEngine::Gui
{
    class ExposedVariableUtils
    {
    public:
        static bool MatchesType(std::string_view typeName, std::string_view expectedSuffix);

        static std::string TrimBraces(std::string_view input);
        static std::vector<float> ExtractNumbers(std::string_view input);

        static std::string CleanTypeName(std::string typeName);
        static std::string MakeDisplayName(const std::string& rawName);
    };
}
