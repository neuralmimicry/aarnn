// BladderBowelProcessor.h

#pragma once

#include <vector>
#include <memory>
#include <atomic>
#include <thread>
#include "SensoryReceptor.h"

class BladderBowelProcessor {
public:
    BladderBowelProcessor();
    ~BladderBowelProcessor();

    bool initialise();
    void startProcessing();
    void stopProcessing();

    void setBladderReceptors(const std::vector<std::shared_ptr<SensoryReceptor>>& receptors);
    void setBowelReceptors(const std::vector<std::shared_ptr<SensoryReceptor>>& receptors);

    void consumeFluid(double amount); // Simulate fluid intake
    void consumeFood(double amount);  // Simulate food intake
    void urinate();                   // Simulate urination
    void defecate();                  // Simulate defecation

private:
    std::vector<std::shared_ptr<SensoryReceptor>> bladderReceptors;
    std::vector<std::shared_ptr<SensoryReceptor>> bowelReceptors;

    // Thread management
    std::atomic<bool> processing;
    std::thread processingThread;

    // Internal state
    std::atomic<double> bladderLevel;
    std::atomic<double> bowelLevel;

    // Internal methods
    void simulateBladderBowelSensations();
    void processBladderBowelData(double bladderLevel, double bowelLevel);
};
