//
// Created by pbisaacs on 23/06/24.
//

#ifndef AARNN_CONFIG_H
#define AARNN_CONFIG_H

#include <map>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include "utils.h"

template <typename T>
std::map<std::string, std::string> read_config(const std::vector<T>& filenames) {
    std::map<std::string, std::string> config;
    for (const auto& filename : filenames) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open file: " << filename << std::endl;
            continue;
        }
        std::string line;
        while (std::getline(file, line)) {
            std::string key, value;
            std::istringstream line_stream(line);
            if (std::getline(line_stream, key, '=') && std::getline(line_stream, value)) {
                config[key] = value;
                std::cout << "Read config: " << key << " = " << value << std::endl;
            }
        }
    }
    return config;
}

#endif //AARNN_CONFIG_H
