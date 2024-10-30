// ChemoreceptionProcessor.h

#pragma once

#include <vector>
#include <memory>
#include <atomic>
#include <thread>
#include "SensoryReceptor.h"

class ChemoreceptionProcessor {
public:
    ChemoreceptionProcessor();
    ~ChemoreceptionProcessor();

    bool initialise();
    void startProcessing();
    void stopProcessing();

    void setChemoreceptiveReceptors(const std::vector<std::shared_ptr<SensoryReceptor>>& receptors);

private:
    std::vector<std::shared_ptr<SensoryReceptor>> chemoreceptiveReceptors;

    // Thread management
    std::atomic<bool> processing;
    std::thread processingThread;

    // Internal methods
    void simulateChemicalChanges();
    void processChemicalData(const std::vector<double>& chemicalLevels);
    void stimulateReceptors(const std::vector<double>& levels);
};
