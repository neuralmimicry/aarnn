// PressureProcessor.h

#pragma once

#include <vector>
#include <memory>
#include <atomic>
#include <thread>
#include "SensoryReceptor.h"

class PressureProcessor {
public:
    PressureProcessor();
    ~PressureProcessor();

    bool initialise();
    void startProcessing();
    void stopProcessing();

    void setPressureReceptors(const std::vector<std::shared_ptr<SensoryReceptor>>& receptors);

private:
    std::vector<std::shared_ptr<SensoryReceptor>> pressureReceptors;

    // Thread management
    std::atomic<bool> processing;
    std::thread processingThread;

    // Internal methods
    void simulatePressureChanges();
    void processPressureData(const std::vector<double>& pressureLevels);
    void stimulateReceptors(const std::vector<double>& levels);
};
