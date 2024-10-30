// SomatosensoryProcessor.h

#pragma once

#include <vector>
#include <memory>
#include <atomic>
#include <thread>
#include "SensoryReceptor.h"

class SomatosensoryProcessor {
public:
    SomatosensoryProcessor();
    ~SomatosensoryProcessor();

    bool initialise();
    void startProcessing();
    void stopProcessing();

    void setTouchReceptors(const std::vector<std::shared_ptr<SensoryReceptor>>& receptors);
    void setTemperatureReceptors(const std::vector<std::shared_ptr<SensoryReceptor>>& receptors);
    void setPainReceptors(const std::vector<std::shared_ptr<SensoryReceptor>>& receptors);

private:
    std::vector<std::shared_ptr<SensoryReceptor>> touchReceptors;
    std::vector<std::shared_ptr<SensoryReceptor>> temperatureReceptors;
    std::vector<std::shared_ptr<SensoryReceptor>> painReceptors;

    // Thread management
    std::atomic<bool> processing;
    std::thread processingThread;

    // Internal methods
    void simulateSomatosensoryInput();
    void processSomatosensoryData(/* parameters */);

    // Helper methods
    void stimulateReceptors(/* processed data */);
};
