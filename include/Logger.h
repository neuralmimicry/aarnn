#ifndef LOGGER_H
#define LOGGER_H

class Logger {
public:
    explicit Logger(const std::string &filename) ;
    ~Logger() ;
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