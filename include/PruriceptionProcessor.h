// PruriceptionProcessor.h

#pragma once

#include <vector>
#include <memory>
#include <atomic>
#include <thread>
#include "SensoryReceptor.h"

class PruriceptionProcessor {
public:
    PruriceptionProcessor();
    ~PruriceptionProcessor();

    bool initialise();
    void startProcessing();
    void stopProcessing();

    void setPruriceptiveReceptors(const std::vector<std::shared_ptr<SensoryReceptor>>& receptors);

private:
    std::vector<std::shared_ptr<SensoryReceptor>> pruriceptiveReceptors;

    // Thread management
    std::atomic<bool> processing;
    std::thread processingThread;

    // Internal methods
    void simulateItchDetection();
    void processItchData(const std::vector<double>& itchStimuli);
    void stimulateReceptors(const std::vector<double>& stimuli);
};
