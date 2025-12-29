#pragma once
#include <filesystem>

namespace BixEngine::Core
{
    std::filesystem::path FindToolExecutable(const std::string& toolName);
    
     
    std::string OpenFileDialog(const char* filter);
}
