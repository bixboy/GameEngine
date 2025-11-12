#include "Utils/StringUtils.h"
#include <algorithm>
#include <windows.h>


namespace BixEngine::StringUtils
{
    // ─────────────────────────────────────────────
    // Chaînes de caractères
    // ─────────────────────────────────────────────

    std::string ToLowerCopy(std::string value)
    {
        std::transform(value.begin(), value.end(), value.begin(),
           [](unsigned char ch)
           {
               return static_cast<char>(std::tolower(ch));
           });

        return value;
    }

    std::string ExtractJsonString(const std::string& source, std::string_view key)
    {
        const std::string pattern = "\"" + std::string(key) + "\"";
        size_t pos = source.find(pattern);
        if (pos == std::string::npos)
            return {};

        pos = source.find(':', pos);
        if (pos == std::string::npos)
            return {};

        pos = source.find('"', pos);
        if (pos == std::string::npos)
            return {};

        const size_t end = source.find('"', pos + 1);
        if (end == std::string::npos)
            return {};

        return source.substr(pos + 1, end - (pos + 1));
    }

    bool MatchesSearch(const String& value, const String& query)
    {
        if (query.IsEmpty())
            return true;

        const String valueLower = ToLowerCopy(value);
        const String queryLower = ToLowerCopy(query);

        return valueLower.find(queryLower.View()) != std::string::npos;
    }

    // ─────────────────────────────────────────────
    // Fichiers et dossiers
    // ─────────────────────────────────────────────

    void ShowPathInExplorer(const std::filesystem::path& path, bool isDirectory)
    {
        if (path.empty())
            return;

        std::string command = "explorer ";
        if (isDirectory)
        {
            command += "\"" + path.string() + "\"";
        }
        else
        {
            command += "/select,\"" + path.string() + "\"";
        }

        std::system(command.c_str());
    }
}
