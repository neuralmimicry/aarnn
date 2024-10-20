#ifndef AARNN_LOGGER_H
#define AARNN_LOGGER_H

#include <chrono>
#include <fstream>
#include <string>
#include <mutex>
#include <stdexcept>
#include <iostream>
#include <iomanip> // For std::put_time

class Logger {
public:
    explicit Logger(const std::string& filename);
    Logger() = delete;
    ~Logger();

    template<typename T>
    Logger& operator<<(const T& msg) {
        std::lock_guard<std::mutex> lock(log_mutex_);

        // Get current time
        auto now = std::chrono::system_clock::now();
        auto now_time_t = std::chrono::system_clock::to_time_t(now);
        auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()) % 1000;

        // Write timestamp
        log_file_ << std::put_time(std::localtime(&now_time_t), "%Y-%m-%d %H:%M:%S")
                  << '.' << std::setfill('0') << std::setw(3) << now_ms.count() << " - ";

        // Write the actual message
        log_file_ << msg;
        log_file_.flush();
        return *this;
    }

    Logger& operator<<(std::ostream& (*pf)(std::ostream&));

private:
    std::ofstream log_file_;
    std::mutex log_mutex_;
};

#endif // AARNN_LOGGER_H
