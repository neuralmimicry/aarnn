// GustatoryProcessor.h

#pragma once

#include <vector>
#include <memory>
#include <atomic>
#include <thread>
#include "SensoryReceptor.h"

class GustatoryProcessor {
public:
    GustatoryProcessor();
    ~GustatoryProcessor();

    bool initialise();
    void startProcessing();
    void stopProcessing();

    void setGustatoryReceptors(const std::vector<std::shared_ptr<SensoryReceptor>>& receptors);

private:
    std::vector<std::shared_ptr<SensoryReceptor>> gustatoryReceptors;

    // Thread management
    std::atomic<bool> processing;
    std::thread processingThread;

    // Internal methods
    void simulateTasteDetection();
    void processTasteData(/* parameters */);

    // Helper methods
    void stimulateReceptors(/* processed data */);
};
