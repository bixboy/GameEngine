#include "Utils/String/StringUtils.h"
#include <algorithm>
#include <windows.h>
#include <cctype>
#include <cstdlib>

namespace BixEngine::StringUtils
{
    // ─────────────────────────────────────────────
    // Chaînes de caractères
    // ─────────────────────────────────────────────

    std::string Utilities::ToLowerCopy(std::string value)
    {
        std::transform(value.begin(), value.end(), value.begin(),
            [](unsigned char ch)
            {
                return static_cast<char>(std::tolower(ch));
            });
        
        return value;
    }

    std::string Utilities::ExtractJsonString(const std::string& source, std::string_view key)
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

        size_t end = source.find('"', pos + 1);
        if (end == std::string::npos)
            return {};

        return source.substr(pos + 1, end - (pos + 1));
    }

    bool Utilities::MatchesSearch(const String& value, const String& query)
    {
        if (query.IsEmpty())
            return true;

        String valueLower = ToLowerCopy(value);
        String queryLower = ToLowerCopy(query);

        return valueLower.find(queryLower.View()) != std::string::npos;
    }

    // ─────────────────────────────────────────────
    // Fichiers et dossiers
    // ─────────────────────────────────────────────

    void Utilities::ShowPathInExplorer(const std::filesystem::path& path, bool isDirectory)
    {
        if (path.empty())
            return;

        std::string command = "explorer ";
        if (isDirectory)
            command += "\"" + path.string() + "\"";
        else
            command += "/select,\"" + path.string() + "\"";

        std::system(command.c_str());
    }

    // ─────────────────────────────────────────────
    // Transforme un path en identifiant string
    // ─────────────────────────────────────────────

    String Utilities::MakeSafeIdentifier(const std::string& raw)
    {
        String out;
        out.reserve(raw.size());

        for (char ch : raw)
        {
            if (std::isalnum(static_cast<unsigned char>(ch)))
                out += static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
            else
                out += '_';
        }

        if (out.IsEmpty()) out = "identifier";
        return out;
    }

    // ─────────────────────────────────────────────
    // Supprime les caractères '\r'
    // ─────────────────────────────────────────────

    void Utilities::TrimCarriageReturn(std::string& v)
    {
        if (!v.empty() && v.back() == '\r')
            v.pop_back();
    }
}
