// SatietyProcessor.h

#pragma once

#include <vector>
#include <memory>
#include <atomic>
#include <thread>
#include "SensoryReceptor.h"

class SatietyProcessor {
public:
    SatietyProcessor();
    ~SatietyProcessor();

    bool initialise();
    void startProcessing();
    void stopProcessing();

    void setSatietyReceptors(const std::vector<std::shared_ptr<SensoryReceptor>>& receptors);
    void consumeFood(double amount); // Simulate food intake

private:
    std::vector<std::shared_ptr<SensoryReceptor>> satietyReceptors;

    // Thread management
    std::atomic<bool> processing;
    std::thread processingThread;

    // Internal state
    std::atomic<double> satietyLevel;

    // Internal methods
    void simulateSatietySignals();
    void processSatietyData(double satietyLevel);
};
