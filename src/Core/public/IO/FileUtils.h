#pragma once
#include <filesystem>

namespace BixEngine::Core
{
    std::filesystem::path FindToolExecutable(const std::string& toolName);
    
    /** 
     * Opens a native file dialog to select a file.
     * @param filter Filter string (e.g. "Scene Files\0*.bix\0All Files\0*.*\0")
     * @return Absolute path to selected file, or empty string if cancelled.
     */
    std::string OpenFileDialog(const char* filter);
}
