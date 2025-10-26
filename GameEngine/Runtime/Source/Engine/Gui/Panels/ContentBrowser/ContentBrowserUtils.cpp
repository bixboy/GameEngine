#include "Core/Logger.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <string>

namespace BixEngine::Gui
{
    void LogAndStoreError(String& storage, String message, bool log)
    {
        if (log)
            LOG_ERROR(message);

        storage = std::move(message);
    }

    String ToLowerCopy(const String& value)
    {
        String result(value);
        std::transform(result.begin(), result.end(), result.begin(), [](unsigned char character)
        {
            return static_cast<char>(std::tolower(character));
        });

        return result;
    }

    bool CaseInsensitiveLess(const String& lhs, const String& rhs)
    {
        const String lhsLower = ToLowerCopy(lhs);
        const String rhsLower = ToLowerCopy(rhs);
        if (lhsLower == rhsLower)
            return lhs.View() < rhs.View();

        return lhsLower.View() < rhsLower.View();
    }

    String TrimCopy(String value)
    {
        const auto isSpace = [](unsigned char character)
        {
            return std::isspace(character) != 0;
        };

        String::size_type start = 0;
        const String::size_type length = value.size();

        while (start < length && isSpace(static_cast<unsigned char>(value[start])))
            ++start;

        String::size_type end = length;
        while (end > start && isSpace(static_cast<unsigned char>(value[end - 1])))
            --end;

        return value.Mid(start, end - start);
    }

    bool ContainsPathSeparator(const String& value)
    {
        const std::string_view view = value.View();
        return view.find('/') != std::string::npos || view.find('\\') != std::string::npos;
    }

    void ShowPathInExplorer(const std::filesystem::path& path, bool isDirectory)
    {
        namespace fs = std::filesystem;

        if (path.empty())
            return;

#ifdef _WIN32
        std::string command = "explorer ";
        if (isDirectory)
        {
            command += '"';
            command += path.string();
            command += '"';
        }
        else
        {
            command += "/select,\"";
            command += path.string();
            command += "\"";
        }
        std::system(command.c_str());
#else
        const fs::path target = isDirectory ? path : path.parent_path();
        if (target.empty())
            return;

        std::string command = "xdg-open \"";
        command += target.string();
        command += "\"";
        std::system(command.c_str());
#endif
    }

    bool MatchesSearch(const String& value, const String& query)
    {
        if (query.IsEmpty())
            return true;

        const String valueLower = ToLowerCopy(value);
        const String queryLower = ToLowerCopy(query);
        return valueLower.find(queryLower.View()) != std::string::npos;
    }
}
