#include "Engine/Gui/Panels/ContentBrowser/ContentBrowserFileUtils.h"

#include "Core/Logger.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <system_error>

namespace BixEngine::Gui
{
    // ─────────────────────────────────────────────
    // Gestion centralisée des erreurs et fichiers
    // ─────────────────────────────────────────────

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
        {
            ++start;   
        }

        String::size_type end = length;
        while (end > start && isSpace(static_cast<unsigned char>(value[end - 1])))
        {
            --end;   
        }

        return value.Mid(start, end - start);
    }

    bool ContainsPathSeparator(const String& value)
    {
        const std::string_view view = value.View();
        return view.find('/') != std::string::npos || view.find('\\') != std::string::npos;
    }

    bool MatchesSearch(const String& value, const String& query)
    {
        if (query.IsEmpty())
            return true;

        const String valueLower = ToLowerCopy(value);
        const String queryLower = ToLowerCopy(query);
        return valueLower.find(queryLower.View()) != std::string::npos;
    }

    void ShowPathInExplorer(const std::filesystem::path& path, bool isDirectory)
    {
        namespace fs = std::filesystem;

        if (path.empty())
            return;

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
    }

    namespace
    {
        bool FormatError(const std::error_code& error, String& storage)
        {
            if (!error)
                return true;

            String message = error.message();
            LogAndStoreError(storage, std::move(message));
            
            return false;
        }
    }

    bool TryCreateDir(const std::filesystem::path& path, String& errorStorage)
    {
        if (path.empty())
        {
            LogAndStoreError(errorStorage, "Directory path is empty.", false);
            return false;
        }

        std::error_code createError;
        std::filesystem::create_directories(path, createError);
        return FormatError(createError, errorStorage);
    }

    bool TryRename(const std::filesystem::path& from, const std::filesystem::path& to, String& errorStorage)
    {
        if (from.empty() || to.empty())
        {
            LogAndStoreError(errorStorage, "Invalid paths for rename.", false);
            return false;
        }

        std::error_code renameError;
        std::filesystem::rename(from, to, renameError);
        return FormatError(renameError, errorStorage);
    }

    bool TryRemove(const std::filesystem::path& target, bool recursive, String& errorStorage)
    {
        if (target.empty())
        {
            LogAndStoreError(errorStorage, "Target path is empty.", false);
            return false;
        }

        std::error_code removeError;
        if (recursive)
        {
            std::filesystem::remove_all(target, removeError);
        }
        else
        {
            std::filesystem::remove(target, removeError);   
        }
        
        return FormatError(removeError, errorStorage);
    }

    bool TryWriteFile(const std::filesystem::path& target, std::string_view contents, String& errorStorage)
    {
        if (target.empty())
        {
            LogAndStoreError(errorStorage, "Target file path is empty.", false);
            return false;
        }

        std::ofstream file(target);
        if (!file.is_open())
        {
            LogAndStoreError(errorStorage, "Unable to open file for writing.");
            return false;
        }

        file.write(contents.data(), contents.size());
        if (!file.good())
        {
            LogAndStoreError(errorStorage, "Failed to write file contents.");
            return false;
        }

        return true;
    }

    bool TryCopyFile(const std::filesystem::path& from, const std::filesystem::path& to, String& errorStorage)
    {
        if (from.empty() || to.empty())
        {
            LogAndStoreError(errorStorage, "Invalid paths for copy.", false);
            return false;
        }

        std::error_code copyError;
        std::filesystem::copy_file(from, to, std::filesystem::copy_options::overwrite_existing, copyError);
        return FormatError(copyError, errorStorage);
    }
}

