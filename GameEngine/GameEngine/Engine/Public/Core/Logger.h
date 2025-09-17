#pragma once
#include "string"

enum class LogLevel {
    Info,
    Warning,
    Error
};

class Logger {
    
public:
    static void Log(const std::string& msg, LogLevel level, const char* file, int line);

private:
    static const char* ExtractFileName(const char* path);
};

// Macros
#define LOG_INFO(msg)    Logger::Log(msg, LogLevel::Info,    __FILE__, __LINE__)
#define LOG_WARNING(msg) Logger::Log(msg, LogLevel::Warning, __FILE__, __LINE__)
#define LOG_ERROR(msg)   Logger::Log(msg, LogLevel::Error,   __FILE__, __LINE__)
