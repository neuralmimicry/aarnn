// AuditoryProcessor.h

#pragma once

#include <vector>
#include <memory>
#include <atomic>
#include <thread>
#include "NetworkClient.h"
#include "SensoryReceptor.h"

class AuditoryProcessor {
public:
    AuditoryProcessor();
    ~AuditoryProcessor();

    bool initialise();
    void startProcessing();
    void stopProcessing();

    void setAuditoryReceptors(const std::vector<std::shared_ptr<SensoryReceptor>>& leftReceptors,
                              const std::vector<std::shared_ptr<SensoryReceptor>>& rightReceptors);

private:
    std::vector<std::shared_ptr<SensoryReceptor>> leftAuditoryReceptors;
    std::vector<std::shared_ptr<SensoryReceptor>> rightAuditoryReceptors;

    // Thread management
    std::atomic<bool> processing;
    std::thread processingThread;

    // Internal methods
    void captureAuditoryData();
    void processAuditoryData(const std::vector<double>& leftChannel,
                          const std::vector<double>& rightChannel);

    // Helper methods
    void stimulateReceptors(const std::vector<double>& leftFrequencies,
                            const std::vector<double>& rightFrequencies);
    // Network client for sending stimuli data
    std::unique_ptr<NetworkClient> networkClient;
};
