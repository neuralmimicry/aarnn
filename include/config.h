#ifndef CONFIG_H_INCLUDED
#define CONFIG_H_INCLUDED

#include <exception>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
// #define DO_TRACE_
#include "traceutil.h"

class Config
{
    public:
    /**
     * @brief Read configuration files.
     * @param filenames Vector of filenames to read the configuration from.
     */
    void read(const std::vector<std::string>& filenames)
    {
        for(const auto& filename: filenames)
        {
            try
            {
                std::ifstream file(filename);
                std::string   line;

                while(std::getline(file, line))
                {
                    std::string        key, value;
                    std::istringstream line_stream(line);
                    std::getline(line_stream, key, '=');
                    std::getline(line_stream, value);
                    config_map_[key] = value;
                }
            }
            catch(const std::exception& e)
            {
                std::stringstream ss;
                ss << "error reading line from '" << filename << "'. Does the file exist? <" << e.what() << ">";

                throw std::runtime_error(ss.str());
            }
        }
    }

    Config(const std::vector<std::string>& filenames = std::vector<std::string>())
    {
        read(filenames);
    }

    std::string operator[](const std::string& key) const
    {
        const auto& found = config_map_.find(key);
        if(found != config_map_.end())
            return found->second;
        std::stringstream ss;
        ss << "configuration key '" << key << "' not defined";

        throw std::runtime_error(ss.str());
    };

    private:
    std::unordered_map<std::string, std::string> config_map_;
};

#endif  // CONFIG_H_INCLUDED
