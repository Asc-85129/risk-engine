#include "Logger.hpp"
#include <ctime>
#include <iomanip>
#include <sstream>

namespace hft {

Logger& Logger::instance() {
    static Logger inst;
    return inst;
}

void Logger::setFile(const std::string& path) {
    std::lock_guard<std::mutex> lk(mtx_);
    file_.open(path, std::ios::app);
}

std::string Logger::timestamp() {
    auto now = std::chrono::system_clock::now();
    auto t   = std::chrono::system_clock::to_time_t(now);
    auto ms  = std::chrono::duration_cast<std::chrono::milliseconds>(
                   now.time_since_epoch()).count() % 1000;
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&t), "%H:%M:%S")
        << "." << std::setw(3) << std::setfill('0') << ms;
    return oss.str();
}

std::string Logger::levelStr(LogLevel lvl) {
    switch (lvl) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO ";
        case LogLevel::WARN:  return "WARN ";
        case LogLevel::ERROR: return "ERROR";
    }
    return "?????";
}

void Logger::log(LogLevel lvl, const std::string& msg) {
    if (lvl < level_) return;
    std::lock_guard<std::mutex> lk(mtx_);
    std::string line = "[" + timestamp() + "][" + levelStr(lvl) + "] " + msg + "\n";
    std::cout << line;
    if (file_.is_open()) file_ << line;
}

} // namespace hft
