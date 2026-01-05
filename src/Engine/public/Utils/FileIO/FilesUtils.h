#pragma once
#include <filesystem>
#include <string>
#include <vector>
#include "Containers/String.h"

namespace BixEngine::Utils
{
    namespace fs = std::filesystem;

    class FileUtils
    {
    public:
        
        static bool TryCreateDir(const fs::path& dir, String& outError);
        
        static bool EnsureDirectory(const fs::path& dir);
        static bool EnsureParentDirectory(const fs::path& filePath);
        
        static bool TryRemove(const fs::path& path, bool recursive, String& outError);
        static bool TryCopyFile(const fs::path& source, const fs::path& destination, bool overwrite, String& outError);
        static bool TryRename(const fs::path& source, const fs::path& destination, bool overwrite, String& outError);


        static bool TryWriteFile(const fs::path& path, const std::string& content, String& outError);
        
        static bool ReadFile(const fs::path& path, String& outContent);
        
        static std::vector<fs::path> ScanDirectory(const fs::path& directory, const std::vector<std::string>& extensions, bool recursive = true);
        
        static bool IsExtension(const fs::path& path, const std::string& extension);
        static bool IsPng(const fs::path& path) { return IsExtension(path, ".png"); }

        static String ExtractDisplayName(const fs::path& path);
        static fs::path NormalizePath(const fs::path& path);
        static String NormalizePath(const String& path);
        static fs::path ResolveUserConfigPath(const char* fileName);

        static bool LogAndStoreError(String& outError, const String& message, bool alsoLog = true);
        static bool CaseInsensitiveLess(const std::string& a, const std::string& b);
    };
}