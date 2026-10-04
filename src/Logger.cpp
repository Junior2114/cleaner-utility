#include "Logger.h"
#include <iostream>
#include "sstream"
#include <chrono>
#include <ctime>
#include <iomanip>

Logger& Logger::instance() {
    static Logger inst;
    return inst;
}

void Logger::setFile(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    file_.open(path, std::ios::app);
}

const char* Logger::levelToString(LogLevel lvl) {
    switch (lvl) {
        case LogLevel::Info:    return "INFO";
        case LogLevel::Warning: return "WARN";
        case LogLevel::Error:   return "ERROR";
    }
    return "?";
}

void Logger::log(LogLevel level, const std::string& msg) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Текущее время
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
    localtime_s(&tm, &t);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S")
        << " [" << levelToString(level) << "] " << msg;

    std::cout << oss.str() << '\n';
    if (file_.is_open()) {
        file_ << oss.str() << '\n';
        file_.flush();
    }
}