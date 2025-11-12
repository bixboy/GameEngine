#pragma once
#include <string_view>

#include "Core/Containers/String.h"

namespace BixEngine::Core
{
    enum class LogLevel
    {
        Info,
        Warning,
        Error
    };

    class Logger
    {
        public:
        static void Log(const String& message, LogLevel level, const char* file, int line);

        private:
            static const char* ExtractFileName(const char* path) noexcept;
        
    };
}

#define LOG_INFO(msg)    ::BixEngine::Core::Logger::Log((msg), ::BixEngine::Core::LogLevel::Info,    __FILE__, __LINE__)
#define LOG_WARNING(msg) ::BixEngine::Core::Logger::Log((msg), ::BixEngine::Core::LogLevel::Warning, __FILE__, __LINE__)
#define LOG_ERROR(msg)   ::BixEngine::Core::Logger::Log((msg), ::BixEngine::Core::LogLevel::Error,   __FILE__, __LINE__)
