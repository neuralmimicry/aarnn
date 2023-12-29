#include <iostream>
#include "../include/Logger.h"


explicit Logger::Logger(const std::string &filename) : log_file(filename, std::ofstream::out | std::ofstream::app) {}

~Logger::Logger() {log_file.close();}