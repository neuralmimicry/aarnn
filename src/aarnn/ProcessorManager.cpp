// ProcessorManager.cpp
#include "ProcessorManager.h"
#include <fstream>
#include <iostream>

namespace json = boost::json;

ProcessorManager::ProcessorManager() {}

ProcessorManager::~ProcessorManager() {
    stopAll();
}

void ProcessorManager::registerAuditoryProcessor(const std::string& id, const std::string& host, unsigned short port) {
    auto processor = std::make_shared<AuditoryProcessor>(host, port, id);
    if (processor->initialise()) {
        std::lock_guard<std::mutex> lock(processorMutex);
        auditoryProcessors[id] = processor;
    } else {
        std::cerr << "Failed to initialise auditory processor: " << id << std::endl;
    }
}

void ProcessorManager::registerVisualProcessor(const std::string& id, const std::string& host, unsigned short port) {
    auto processor = std::make_shared<VisualProcessor>(host, port, id);
    if (processor->initialise()) {
        std::lock_guard<std::mutex> lock(processorMutex);
        visualProcessors[id] = processor;
    } else {
        std::cerr << "Failed to initialise visual processor: " << id << std::endl;
    }
}

// ------------------------------------------------------------------------------------------------
// void ProcessorManager::loadFromJsonConfig(const std::string& filePath)
// ------------------------------------------------------------------------------------------------
void ProcessorManager::loadFromJsonConfig(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Failed to open config file: " << filePath << std::endl;
        return;
    }

    // Read entire file into a string
    std::string jsonText;
    {
        std::ostringstream ss;
        ss << file.rdbuf();
        jsonText = ss.str();
    }

    // Parse into a boost::json::value and get the root object
    boost::json::error_code ec;
    boost::json::value parsed = boost::json::parse(jsonText, ec);
    if (ec) {
        std::cerr << "Failed to parse JSON config: " << ec.message() << std::endl;
        return;
    }
    const auto& rootObj = parsed.as_object();

    // Handle "auditory" array, if present
    if (rootObj.if_contains("auditory") && rootObj.at("auditory").is_array()) {
        const auto& auditoryArray = rootObj.at("auditory").as_array();
        for (const auto& element : auditoryArray) {
            if (!element.is_object()) {
                continue; // skip non-object entries
            }
            const auto& procObj = element.as_object();

            // Extract "id"
            if (!procObj.if_contains("id") || !procObj.at("id").is_string()) {
                continue;
            }
            std::string id = procObj.at("id").as_string().c_str();

            // Extract "host"
            if (!procObj.if_contains("host") || !procObj.at("host").is_string()) {
                continue;
            }
            std::string host = procObj.at("host").as_string().c_str();

            // Extract "port"
            if (!procObj.if_contains("port") || !procObj.at("port").is_int64()) {
                continue;
            }
            unsigned short port = static_cast<unsigned short>(procObj.at("port").as_int64());

            registerAuditoryProcessor(id, host, port);
        }
    }

    // Handle "visual" array, if present
    if (rootObj.if_contains("visual") && rootObj.at("visual").is_array()) {
        const auto& visualArray = rootObj.at("visual").as_array();
        for (const auto& element : visualArray) {
            if (!element.is_object()) {
                continue; // skip non-object entries
            }
            const auto& procObj = element.as_object();

            // Extract "id"
            if (!procObj.if_contains("id") || !procObj.at("id").is_string()) {
                continue;
            }
            std::string id = procObj.at("id").as_string().c_str();

            // Extract "host"
            if (!procObj.if_contains("host") || !procObj.at("host").is_string()) {
                continue;
            }
            std::string host = procObj.at("host").as_string().c_str();

            // Extract "port"
            if (!procObj.if_contains("port") || !procObj.at("port").is_int64()) {
                continue;
            }
            unsigned short port = static_cast<unsigned short>(procObj.at("port").as_int64());

            registerVisualProcessor(id, host, port);
        }
    }
}

void ProcessorManager::startAll() {
    std::lock_guard<std::mutex> lock(processorMutex);
    for (auto& [id, processor] : auditoryProcessors) {
        processor->startProcessing();
    }
    for (auto& [id, processor] : visualProcessors) {
        processor->startProcessing();
    }
    monitoring = true;
    healthMonitorThread = std::thread(&ProcessorManager::monitorHealth, this);
}

void ProcessorManager::stopAll() {
    monitoring = false;
    if (healthMonitorThread.joinable()) {
        healthMonitorThread.join();
    }
    std::lock_guard<std::mutex> lock(processorMutex);
    for (auto& [id, processor] : auditoryProcessors) {
        processor->stopProcessing();
    }
    for (auto& [id, processor] : visualProcessors) {
        processor->stopProcessing();
    }
    auditoryProcessors.clear();
    visualProcessors.clear();
}

void ProcessorManager::monitorHealth() {
    while (monitoring.load()) {
        {
            std::lock_guard<std::mutex> lock(processorMutex);
            for (auto& [id, processor] : auditoryProcessors) {
                if (!processor->isHealthy()) {
                    std::cerr << "[HealthCheck] Auditory processor " << id << " is unhealthy. Attempting restart..." << std::endl;
                    processor->stopProcessing();
                    if (processor->initialise()) {
                        processor->startProcessing();
                        std::cout << "[HealthCheck] Auditory processor " << id << " recovered." << std::endl;
                    } else {
                        std::cerr << "[HealthCheck] Auditory processor " << id << " failed to recover." << std::endl;
                    }
                }
            }
            for (auto& [id, processor] : visualProcessors) {
                if (!processor->isHealthy()) {
                    std::cerr << "[HealthCheck] Visual processor " << id << " is unhealthy. Attempting restart..." << std::endl;
                    processor->stopProcessing();
                    if (processor->initialise()) {
                        processor->startProcessing();
                        std::cout << "[HealthCheck] Visual processor " << id << " recovered." << std::endl;
                    } else {
                        std::cerr << "[HealthCheck] Visual processor " << id << " failed to recover." << std::endl;
                    }
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}

// ------------------------------------------------------------------------------------------------
// boost::json::object ProcessorManager::getProcessorStatuses() const
// ------------------------------------------------------------------------------------------------
boost::json::object ProcessorManager::getProcessorStatuses() const {
    boost::json::object result;
    std::lock_guard<std::mutex> lock(processorMutex);

    // Build an array of auditory statuses
    boost::json::array auditoryArray;
    for (const auto& [id, processor] : auditoryProcessors) {
        boost::json::object entry;
        entry["id"] = id;
        entry["healthy"] = processor->isHealthy();
        auditoryArray.push_back(std::move(entry));
    }
    result["auditory"] = std::move(auditoryArray);

    // Build an array of visual statuses
    boost::json::array visualArray;
    for (const auto& [id, processor] : visualProcessors) {
        boost::json::object entry;
        entry["id"] = id;
        entry["healthy"] = processor->isHealthy();
        visualArray.push_back(std::move(entry));
    }
    result["visual"] = std::move(visualArray);

    return result;
}