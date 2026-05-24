/*Простой потокобезопасный логгер с записью в файл mcs_log.txt. 
Управляется макросом LOG и флагом ENABLE_LOGGING.*/
#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>

class Logger {
public:
    static Logger& instance();
    void log(const std::string& msg);
    void setEnabled(bool enabled);
    bool isEnabled() const;
private:
    Logger();
    ~Logger();
    std::ofstream file;
    std::mutex mtx;
    bool enabled;
};

#ifdef ENABLE_LOGGING
#define LOG(msg) Logger::instance().log(msg)
#else
#define LOG(msg) ((void)0)
#endif

#endif // LOGGER_H