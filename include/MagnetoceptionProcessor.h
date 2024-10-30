// MagnetoceptionProcessor.h

#pragma once

#include <vector>
#include <memory>
#include <atomic>
#include <thread>
#include "SensoryReceptor.h"

class MagnetoceptionProcessor {
public:
    MagnetoceptionProcessor();
    ~MagnetoceptionProcessor();

    bool initialise();
    void startProcessing();
    void stopProcessing();

    void setMagnetoceptiveReceptors(const std::vector<std::shared_ptr<SensoryReceptor>>& receptors);

private:
    std::vector<std::shared_ptr<SensoryReceptor>> magnetoceptiveReceptors;

    // Thread management
    std::atomic<bool> processing;
    std::thread processingThread;

    // Internal methods
    void simulateMagneticFieldDetection();
    void processMagneticFieldData(const std::vector<double>& magneticFieldStrengths);
    void stimulateReceptors(const std::vector<double>& strengths);
};
