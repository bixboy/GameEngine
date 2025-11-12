#include "Core/FileUtils.h"
#include "Core/Logger.h"
#include <filesystem>
#include <array>

namespace fs = std::filesystem;

namespace BixEngine::Core
{
    std::filesystem::path FindToolExecutable(const std::string& toolName)
    {
        fs::path current = fs::current_path();
        fs::path foundPath;

        for (int depth = 0; depth < 6 && foundPath.empty(); ++depth)
        {
            fs::path buildDir = current / "Build";
            if (fs::exists(buildDir))
            {
                std::array<std::string, 2> modes = { "debug", "release" };
                std::array<std::string, 2> archs = { "x64", "x86" };
                std::array<std::string, 2> plats = { "windows", "linux" };

                for (const auto& plat : plats)
                {
                    for (const auto& arch : archs)
                    {
                        for (const auto& mode : modes)
                        {
                            fs::path candidate = buildDir / plat / arch / mode / toolName;
                            if (fs::exists(candidate))
                            {
                                foundPath = fs::weakly_canonical(candidate);
                                break;
                            }
                        }
                        if (!foundPath.empty()) break;
                    }
                    if (!foundPath.empty()) break;
                }
            }

            current = current.parent_path();
        }

        if (foundPath.empty())
        {
            LOG_WARNING(String("❌ Tool not found: ") + toolName.c_str());
        }
        else
        {
            LOG_INFO(String("✅ Found tool: ") + foundPath.string().c_str());
        }

        return foundPath;
    }
}
