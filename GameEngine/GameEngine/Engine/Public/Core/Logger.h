#pragma once

#include <string>
#include <string_view>

namespace Engine::Core {

enum class LogLevel {
    Info,
    Warning,
    Error
};

class Logger {
public:
    static void Log(std::string_view message, LogLevel level, const char* file, int line);

private:
    static const char* ExtractFileName(const char* path) noexcept;
};

}  // namespace Engine::Core

#define LOG_INFO(msg)    ::Engine::Core::Logger::Log((msg), ::Engine::Core::LogLevel::Info,    __FILE__, __LINE__)
#define LOG_WARNING(msg) ::Engine::Core::Logger::Log((msg), ::Engine::Core::LogLevel::Warning, __FILE__, __LINE__)
#define LOG_ERROR(msg)   ::Engine::Core::Logger::Log((msg), ::Engine::Core::LogLevel::Error,   __FILE__, __LINE__)
