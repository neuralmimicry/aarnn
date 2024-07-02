//
// Created by pbisaacs on 23/06/24.
//

#ifndef AARNN_TIMER_H
#define AARNN_TIMER_H

#include <string>
#include <functional>
#include "Logger.h"

template <typename Func>
void logExecutionTime(Func function, const std::string& functionName) {
    auto start = std::chrono::high_resolution_clock::now();
    function();
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::ofstream logFile("execution_times.log", std::ios_base::app); // Open the log file in append mode
    logFile << functionName << " execution time: " << duration.count() << " microseconds\n";
    logFile.close();
}

#endif //AARNN_TIMER_H
