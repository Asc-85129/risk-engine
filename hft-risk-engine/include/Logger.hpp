#pragma once
#include <string>
#include <fstream>
#include <mutex>
#include <chrono>
#include <iostream>

namespace hft {

enum class LogLevel { DEBUG, INFO, WARN, ERROR };

class Logger {
public:
    static Logger& instance();

    void setLevel(LogLevel lvl)  { level_ = lvl; }
    void setFile(const std::string& path);

    void log(LogLevel lvl, const std::string& msg);

    void debug(const std::string& m) { log(LogLevel::DEBUG, m); }
    void info (const std::string& m) { log(LogLevel::INFO,  m); }
    void warn (const std::string& m) { log(LogLevel::WARN,  m); }
    void error(const std::string& m) { log(LogLevel::ERROR, m); }

private:
    Logger() = default;
    std::mutex    mtx_;
    LogLevel      level_ = LogLevel::INFO;
    std::ofstream file_;

    static std::string levelStr(LogLevel lvl);
    static std::string timestamp();
};

#define LOG_INFO(msg)  hft::Logger::instance().info(msg)
#define LOG_WARN(msg)  hft::Logger::instance().warn(msg)
#define LOG_ERROR(msg) hft::Logger::instance().error(msg)
#define LOG_DEBUG(msg) hft::Logger::instance().debug(msg)

} // namespace hft
