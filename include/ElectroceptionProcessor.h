// ElectroreceptionProcessor.h

#pragma once

#include <vector>
#include <memory>
#include <atomic>
#include <thread>
#include "SensoryReceptor.h"

class ElectroreceptionProcessor {
public:
    ElectroreceptionProcessor();
    ~ElectroreceptionProcessor();

    bool initialise();
    void startProcessing();
    void stopProcessing();

    void setElectroreceptiveReceptors(const std::vector<std::shared_ptr<SensoryReceptor>>& receptors);

private:
    std::vector<std::shared_ptr<SensoryReceptor>> electroreceptiveReceptors;

    // Thread management
    std::atomic<bool> processing;
    std::thread processingThread;

    // Internal methods
    void simulateElectricFieldDetection();
    void processElectricFieldData(const std::vector<double>& electricFieldIntensities);
    void stimulateReceptors(const std::vector<double>& intensities);
};
