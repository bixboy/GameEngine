#pragma once
#include <filesystem>
#include <string>
#include <cstdint>
#include "Containers/String.h"

namespace BixEngine::EditorUtils
{
    class Utilities
    {
    public:

         
        static void SetPreferredCodeEditor(const std::string& command);

         
        static const std::string& GetPreferredCodeEditor();
        

         
        static void OpenFileInCodeEditor(const std::filesystem::path& path);

         
        static void ShowPathInExplorer(const std::filesystem::path& path, bool isDirectory);
        

         
        [[nodiscard]] static std::uint64_t HashFNV1a(std::string_view str);

    private:
        static std::string s_EditorCommand;
        static void DetectDefaultEditor();
        static bool IsExecutableAvailable(const std::string& exeName);
    };
}
