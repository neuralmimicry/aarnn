#include "Logger.h"

Logger::Logger(const std::string& filename)
        : log_file_(filename, std::ofstream::out | std::ofstream::app) {
    if (!log_file_.is_open()) {
        throw std::runtime_error("Failed to open log file: " + filename);
    }
}

Logger::~Logger() {
    if (log_file_.is_open()) {
        log_file_.close();
    }
}

Logger& Logger::operator<<(std::ostream& (*pf)(std::ostream&)) {
    std::lock_guard<std::mutex> lock(log_mutex_);
    log_file_ << pf;
    log_file_.flush();
    return *this;
}
