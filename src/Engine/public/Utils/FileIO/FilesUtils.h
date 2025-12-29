#pragma once
#include <filesystem>
#include <string>
#include "Containers/String.h"


namespace BixEngine::FilesUtils
{
    class Utilities
    {
    public:
        
        
        

        static bool TryCreateDir(const std::filesystem::path& dir, String& outError);
        static bool TryRemove(const std::filesystem::path& path, bool recursive, String& outError);

        
        
        

        static bool TryCopyFile(const std::filesystem::path& source, const std::filesystem::path& destination, bool overwrite, String& outError);

        static bool TryRename(const std::filesystem::path& source, const std::filesystem::path& destination, bool overwrite, String& outError);

        static bool TryWriteFile(const std::filesystem::path& path, const std::string& content, String& outError);

        static std::vector<std::filesystem::path> ScanDirectory(const std::filesystem::path& directory,  const std::vector<std::string>& extensions, bool recursive = true);

        
        
        

        static bool LogAndStoreError(String& outError, const String& message, bool alsoLog = true);
        static bool CaseInsensitiveLess(const std::string& a, const std::string& b);

        
        
        

        static String ExtractDisplayName(const std::filesystem::path& path);
        static std::filesystem::path NormalizePath(const std::filesystem::path& path);
        static std::filesystem::path ResolveUserConfigPath(const char* fileName);
    };
}
