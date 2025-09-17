#include "../../Public/Core/Logger.h"
#include "iostream"

// Color code
#define COLOR_RESET   "\033[0m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_RED     "\033[31m"

// File name extraction
const char* Logger::ExtractFileName(const char* path) {
    
    const char* slash1 = strrchr(path, '/');
    const char* slash2 = strrchr(path, '\\');
    const char* filename = slash1 ? slash1 + 1 : path;
    
    if (slash2 && slash2 > filename)
        filename = slash2 + 1;
    
    return filename;
}

// Print log
void Logger::Log(const std::string& msg, LogLevel level, const char* file, int line) 
{
    const char* color = COLOR_RESET;
    std::string levelStr;
    
    switch (level)
    {
        case LogLevel::Info:
            color = COLOR_GREEN;
            levelStr = "[INFO]";
            break;
        case LogLevel::Warning:
            color = COLOR_YELLOW;
            levelStr = "[WARNING]";
            break;
        case LogLevel::Error:
            color = COLOR_RED;
            levelStr = "[ERROR]";
            break;
    }

    std::cout << color << ExtractFileName(file) << "(" << line << "): " << levelStr << " " << msg << COLOR_RESET << std::endl;
}
