// HungerThirstProcessor.h

#pragma once

#include <vector>
#include <memory>
#include <atomic>
#include <thread>
#include "SensoryReceptor.h"

class HungerThirstProcessor {
public:
    HungerThirstProcessor();
    ~HungerThirstProcessor();

    bool initialise();
    void startProcessing();
    void stopProcessing();

    void setHungerReceptors(const std::vector<std::shared_ptr<SensoryReceptor>>& receptors);
    void setThirstReceptors(const std::vector<std::shared_ptr<SensoryReceptor>>& receptors);

private:
    std::vector<std::shared_ptr<SensoryReceptor>> hungerReceptors;
    std::vector<std::shared_ptr<SensoryReceptor>> thirstReceptors;

    // Thread management
    std::atomic<bool> processing;
    std::thread processingThread;

    // Internal methods
    void simulateHungerThirstSignals();
    void processHungerThirstData(double hungerLevel, double thirstLevel);
};
