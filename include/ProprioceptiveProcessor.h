// ProprioceptiveProcessor.h

#pragma once

#include <vector>
#include <memory>
#include <atomic>
#include <thread>
#include "SensoryReceptor.h"

class ProprioceptiveProcessor {
public:
    ProprioceptiveProcessor();
    ~ProprioceptiveProcessor();

    bool initialise();
    void startProcessing();
    void stopProcessing();

    void setProprioceptiveReceptors(const std::vector<std::shared_ptr<SensoryReceptor>>& receptors);
    void setEquilibrioceptiveReceptors(const std::vector<std::shared_ptr<SensoryReceptor>>& receptors);

private:
    std::vector<std::shared_ptr<SensoryReceptor>> proprioceptiveReceptors;
    std::vector<std::shared_ptr<SensoryReceptor>> equilibrioceptiveReceptors;

    // Thread management
    std::atomic<bool> processing;
    std::thread processingThread;

    // Internal methods
    void simulateProprioceptiveInput();

    // Helper methods
    void stimulateReceptors(const std::vector<double> &proprioceptiveStimuli);

    void processProprioceptiveData(const std::vector<double> &proprioceptiveStimuli,
                                   const std::vector<double> &equilibrioceptiveStimuli);
};
