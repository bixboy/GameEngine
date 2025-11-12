#pragma once
#include <filesystem>
#include "Core/Containers/String.h"

namespace BixEngine::Core
{
    std::filesystem::path FindToolExecutable(const std::string& toolName);
}
