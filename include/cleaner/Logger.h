#pragma once
#include <fstream>
#include <mutex>
#include <string>

enum class LogLevel { Info, Warning, Error };

class Logger {
public:
    static Logger& instance();

    void setFile(const std::string& path);
    void log(LogLevel level, const std::string& msg);

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

private:
    Logger() = default;

    static const char* levelToString(LogLevel lvl);

    std::ofstream file_;
    std::mutex    mutex_;
};