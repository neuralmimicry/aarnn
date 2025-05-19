// ProcessorManager.cpp
#include "ProcessorManager.h"
#include <fstream>
#include <iostream>

using json = nlohmann::json;

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

void ProcessorManager::loadFromJsonConfig(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Failed to open config file: " << filePath << std::endl;
        return;
    }

    json config;
    file >> config;

    for (const auto& processor : config["auditory"]) {
        std::string id = processor["id"];
        std::string host = processor["host"];
        unsigned short port = processor["port"];
        registerAuditoryProcessor(id, host, port);
    }

    for (const auto& processor : config["visual"]) {
        std::string id = processor["id"];
        std::string host = processor["host"];
        unsigned short port = processor["port"];
        registerVisualProcessor(id, host, port);
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

nlohmann::json ProcessorManager::getProcessorStatuses() const {
    nlohmann::json result;
    std::lock_guard<std::mutex> lock(processorMutex);

    for (const auto& [id, processor] : auditoryProcessors) {
        result["auditory"].push_back({
                                             {"id", id},
                                             {"healthy", processor->isHealthy()}
                                     });
    }

    for (const auto& [id, processor] : visualProcessors) {
        result["visual"].push_back({
                                           {"id", id},
                                           {"healthy", processor->isHealthy()}
                                   });
    }

    return result;
}
