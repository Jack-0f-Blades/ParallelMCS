#include "Logger.h"

Logger::Logger() : enabled(false) {
    file.open("mcs_log.txt", std::ios::out | std::ios::trunc);
}

Logger::~Logger() {
    if (file.is_open()) file.close();
}

Logger& Logger::instance() {
    static Logger inst;
    return inst;
}

void Logger::log(const std::string& msg) {
    if (!enabled) return;
    std::lock_guard<std::mutex> lock(mtx);
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    file << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S")
         << '.' << std::setfill('0') << std::setw(3) << ms.count()
         << " " << msg << std::endl;
}

void Logger::setEnabled(bool e) { enabled = e; }
bool Logger::isEnabled() const { return enabled; }