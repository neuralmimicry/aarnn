#ifndef LOGGER_H_INCLUDED
#define LOGGER_H_INCLUDED

#include <fstream>
#include <chrono>

class Logger
{
    public:
    explicit Logger(const std::string &filename) : log_file_(filename, std::ofstream::out | std::ofstream::app)
    {
    }

    ~Logger()
    {
        log_file_.close();
    }

    template<typename T>
    Logger &operator<<(const T &msg)
    {
        log_file_ << msg;
        log_file_.flush();
        return *this;
    }

    Logger &operator<<(std::ostream &(*pf)(std::ostream &))
    {
        log_file_ << pf;
        log_file_.flush();
        return *this;
    }

    template<typename Func>
    static void logExecutionTime(Func function, const std::string &functionName)
    {
        auto start = std::chrono::high_resolution_clock::now();

        function();

        auto end = std::chrono::high_resolution_clock::now();

        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        std::ofstream logFile("execution_times.log", std::ios_base::app);  // Open the log file in append mode
        logFile << functionName << " execution time: " << duration.count() << " microseconds\n";
        logFile.close();
    }

    private:
    std::ofstream log_file_;
};

#endif  // LOGGER_H_INCLUDED