#pragma once

#include "Types.h"
#include <iostream>
#include <sstream>
#include <mutex>

namespace XeSS {

enum class LogLevel : uint32 {
    Trace = 0,
    Debug = 1,
    Info = 2,
    Warning = 3,
    Error = 4,
    Critical = 5
};

class Logger {
public:
    static Logger& Instance();

    void SetLevel(LogLevel level);
    LogLevel GetLevel() const;

    void Log(LogLevel level, const std::string& message);

    template<typename... Args>
    void Trace(const std::string& format, Args&&... args) {
        LogFormatted(LogLevel::Trace, format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void Debug(const std::string& format, Args&&... args) {
        LogFormatted(LogLevel::Debug, format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void Info(const std::string& format, Args&&... args) {
        LogFormatted(LogLevel::Info, format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void Warning(const std::string& format, Args&&... args) {
        LogFormatted(LogLevel::Warning, format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void Error(const std::string& format, Args&&... args) {
        LogFormatted(LogLevel::Error, format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void Critical(const std::string& format, Args&&... args) {
        LogFormatted(LogLevel::Critical, format, std::forward<Args>(args)...);
    }

private:
    Logger() = default;
    ~Logger() = default;

    template<typename... Args>
    void LogFormatted(LogLevel level, const std::string& format, Args&&... args) {
        if (level >= m_level) {
            std::ostringstream oss;
            FormatMessage(oss, format, std::forward<Args>(args)...);
            Log(level, oss.str());
        }
    }

    template<typename T>
    void FormatMessage(std::ostringstream& oss, const std::string& format, T&& value) {
        size_t pos = format.find("{}");
        if (pos != std::string::npos) {
            oss << format.substr(0, pos) << std::forward<T>(value) << format.substr(pos + 2);
        } else {
            oss << format;
        }
    }

    template<typename T, typename... Args>
    void FormatMessage(std::ostringstream& oss, const std::string& format, T&& value, Args&&... args) {
        size_t pos = format.find("{}");
        if (pos != std::string::npos) {
            std::string newFormat = format.substr(0, pos) + std::to_string(value) + format.substr(pos + 2);
            FormatMessage(oss, newFormat, std::forward<Args>(args)...);
        } else {
            oss << format;
        }
    }

    const char* LevelToString(LogLevel level) const;

    LogLevel m_level{LogLevel::Info};
    mutable std::mutex m_mutex;
};

// Convenience macros
#define XESS_TRACE(...) XeSS::Logger::Instance().Trace(__VA_ARGS__)
#define XESS_DEBUG(...) XeSS::Logger::Instance().Debug(__VA_ARGS__)
#define XESS_INFO(...) XeSS::Logger::Instance().Info(__VA_ARGS__)
#define XESS_WARNING(...) XeSS::Logger::Instance().Warning(__VA_ARGS__)
#define XESS_ERROR(...) XeSS::Logger::Instance().Error(__VA_ARGS__)
#define XESS_CRITICAL(...) XeSS::Logger::Instance().Critical(__VA_ARGS__)

} // namespace XeSS
