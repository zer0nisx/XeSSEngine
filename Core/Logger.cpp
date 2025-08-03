#include "Logger.h"
#include <iostream>
#include <chrono>
#include <iomanip>

#ifdef _WIN32
#include <windows.h>
#endif

namespace XeSS {

Logger& Logger::Instance() {
    static Logger instance;
    return instance;
}

void Logger::SetLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_level = level;
}

LogLevel Logger::GetLevel() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_level;
}

void Logger::Log(LogLevel level, const std::string& message) {
    if (level < m_level) {
        return;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    // Get current time
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::ostringstream oss;
    oss << "[" << std::put_time(std::localtime(&time_t), "%H:%M:%S");
    oss << "." << std::setfill('0') << std::setw(3) << ms.count() << "] ";
    oss << "[" << LevelToString(level) << "] ";
    oss << message << std::endl;

    std::string logMessage = oss.str();

    // Output to console
    if (level >= LogLevel::Error) {
        std::cerr << logMessage;
    } else {
        std::cout << logMessage;
    }

#ifdef _WIN32
    // Output to Visual Studio debug console
    OutputDebugStringA(logMessage.c_str());
#endif
}

const char* Logger::LevelToString(LogLevel level) const {
    switch (level) {
        case LogLevel::Trace:    return "TRACE";
        case LogLevel::Debug:    return "DEBUG";
        case LogLevel::Info:     return "INFO ";
        case LogLevel::Warning:  return "WARN ";
        case LogLevel::Error:    return "ERROR";
        case LogLevel::Critical: return "CRIT ";
        default:                 return "UNKN ";
    }
}

} // namespace XeSS
