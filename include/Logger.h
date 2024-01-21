#ifndef LOGGER_H
#define LOGGER_H

#include <fstream>

class Logger {
public:
    explicit Logger(const std::string &filename) : log_file(filename, std::ofstream::out | std::ofstream::app) {}

    Logger() {log_file.close();}
    template<typename T>
    Logger &operator<<(const T &msg) {
        log_file << msg;
        log_file.flush();
        return *this;
    }
    Logger &operator<<(std::ostream& (*pf)(std::ostream&)) {
        log_file << pf;
        log_file.flush();
        return *this;
    }
private:
    std::ofstream log_file;
};


#endif