// ProcessorManager.h
#pragma once

#include <memory>
#include <unordered_map>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <boost/json.hpp>
#include "AuditoryProcessor.h"
#include "VisualProcessor.h"


// Manages a group of sensory processors (e.g. Auditory, Visual).
// Handles lifecycle (initialise, start, stop), health monitoring,
// and provides hooks for periodic diagnostics or recovery.
class ProcessorManager {
public:
    ProcessorManager();
    ~ProcessorManager();

    void registerAuditoryProcessor(const std::string& id, const std::string& host, unsigned short port);
    void registerVisualProcessor(const std::string& id, const std::string& host, unsigned short port);
    void loadFromJsonConfig(const std::string& filePath); // Load processors dynamically

    void startAll();
    void stopAll();

    void monitorHealth(); // Optional background health checker
    // Query the current status of all processors (auditory and visual)
    boost::json::object getProcessorStatuses() const;

private:
    std::unordered_map<std::string, std::shared_ptr<AuditoryProcessor>> auditoryProcessors;
    std::unordered_map<std::string, std::shared_ptr<VisualProcessor>> visualProcessors;

    std::thread healthMonitorThread;
    std::atomic<bool> monitoring{false};
    mutable std::mutex processorMutex;
};
