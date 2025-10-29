#pragma once

#include "Core/Containers/String.h"

#include <filesystem>
#include <string_view>

namespace BixEngine::Gui
{
    void LogAndStoreError(String& storage, String message, bool log = true);

    String ToLowerCopy(const String& value);
    bool CaseInsensitiveLess(const String& lhs, const String& rhs);
    String TrimCopy(String value);
    bool ContainsPathSeparator(const String& value);
    bool MatchesSearch(const String& value, const String& query);

    void ShowPathInExplorer(const std::filesystem::path& path, bool isDirectory);

    bool TryCreateDir(const std::filesystem::path& path, String& errorStorage);
    bool TryRename(const std::filesystem::path& from, const std::filesystem::path& to, String& errorStorage);
    bool TryRemove(const std::filesystem::path& target, bool recursive, String& errorStorage);
    bool TryWriteFile(const std::filesystem::path& target, std::string_view contents, String& errorStorage);
    bool TryCopyFile(const std::filesystem::path& from, const std::filesystem::path& to, String& errorStorage);
}

