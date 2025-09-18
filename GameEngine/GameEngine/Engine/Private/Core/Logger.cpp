#include "Core/Logger.h"
#include <cstring>
#include <iostream>
#include <string_view>

namespace Engine::Core
{
    namespace
    {
        constexpr const char* kColorReset = "\033[0m";
        constexpr const char* kColorGreen = "\033[32m";
        constexpr const char* kColorYellow = "\033[33m";
        constexpr const char* kColorRed = "\033[31m";
    }

    const char* Logger::ExtractFileName(const char* path) noexcept
    {
        const char* slash1 = std::strrchr(path, '/');
        const char* slash2 = std::strrchr(path, '\\');
        const char* filename = slash1 ? slash1 + 1 : path;

        if (slash2 && slash2 > filename)
            filename = slash2 + 1;

        return filename;
    }

    void Logger::Log(std::string_view message, LogLevel level, const char* file, int line)
    {
        const char* color = kColorReset;
        std::string levelStr;

        switch (level)
        {
            case LogLevel::Info:
                color = kColorGreen;
                levelStr = "[INFO]";
                break;
            
            case LogLevel::Warning:
                color = kColorYellow;
                levelStr = "[WARNING]";
                break;
            
            case LogLevel::Error:
                color = kColorRed;
                levelStr = "[ERROR]";
                break;
        }

        std::cout << color << ExtractFileName(file) << "(" << line << ") " << levelStr << ' ' << message << kColorReset << '\n';
    }
}
