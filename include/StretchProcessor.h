// StretchProcessor.h

#pragma once

#include <vector>
#include <memory>
#include <atomic>
#include <thread>
#include "SensoryReceptor.h"

class StretchProcessor {
public:
    StretchProcessor();
    ~StretchProcessor();

    bool initialise();
    void startProcessing();
    void stopProcessing();

    void setStretchReceptors(const std::vector<std::shared_ptr<SensoryReceptor>>& receptors);

private:
    std::vector<std::shared_ptr<SensoryReceptor>> stretchReceptors;

    // Thread management
    std::atomic<bool> processing;
    std::thread processingThread;

    // Internal methods
    void simulateStretchDetection();
    void processStretchData(const std::vector<double>& stretchLevels);
    void stimulateReceptors(const std::vector<double>& levels);
};
