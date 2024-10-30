// InteroceptiveProcessor.h

#pragma once

#include <vector>
#include <memory>
#include <atomic>
#include <thread>
#include "SensoryReceptor.h"

class InteroceptiveProcessor {
public:
    InteroceptiveProcessor();
    ~InteroceptiveProcessor();

    bool initialise();
    void startProcessing();
    void stopProcessing();

    void setInteroceptiveReceptors(const std::vector<std::shared_ptr<SensoryReceptor>>& receptors);

private:
    std::vector<std::shared_ptr<SensoryReceptor>> interoceptiveReceptors;

    // Thread management
    std::atomic<bool> processing;
    std::thread processingThread;

    // Internal methods
    void simulateInteroceptiveInput();
    void processInteroceptiveData(/* parameters */);

    // Helper methods
    void stimulateReceptors(/* processed data */);
};
