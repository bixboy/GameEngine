#pragma once
#include <string>
#include <string_view>
#include <filesystem>
#include "Containers/String.h"

namespace BixEngine::StringUtils
{
    class Utilities
    {
    public:

         
        static std::string ToLowerCopy(std::string value);

         
        static std::string ExtractJsonString(const std::string& source, std::string_view key);

         
        static bool MatchesSearch(const String& value, const String& query);
        

         
        static void ShowPathInExplorer(const std::filesystem::path& path, bool isDirectory);

        
        static String MakeSafeIdentifier(const std::string& raw);

        
        static void TrimCarriageReturn(std::string& v);
    };
}
