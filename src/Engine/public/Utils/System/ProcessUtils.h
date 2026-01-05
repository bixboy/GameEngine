#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include "Containers/String.h"

namespace BixEngine::System
{
    struct ProcessResult
    {
        int exitCode = -1;
        String output;
        bool success = false;
    };

    class ProcessUtils
    {
    public:
        // Lance un processus et attend qu'il finisse
        static ProcessResult RunCommand(const std::filesystem::path& executable, const std::vector<String>& args);

        // Lance un processus et rend la main tout de suite
        static bool LaunchDetached(const std::filesystem::path& executable, const std::string& args);
    };
}