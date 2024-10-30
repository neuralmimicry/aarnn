// HungerThirstProcessor.cpp

#include "HungerThirstProcessor.h"
#include <chrono>
#include <algorithm>

HungerThirstProcessor::HungerThirstProcessor() : processing(false) {}

HungerThirstProcessor::~HungerThirstProcessor() {
    stopProcessing();
}

bool HungerThirstProcessor::initialise() {
    // Initialize simulation parameters
    return true;
}

void HungerThirstProcessor::startProcessing() {
    if (!processing.load()) {
        processing = true;
        processingThread = std::thread(&HungerThirstProcessor::simulateHungerThirstSignals, this);
    }
}

void HungerThirstProcessor::stopProcessing() {
    if (processing.load()) {
        processing = false;
        if (processingThread.joinable()) {
            processingThread.join();
        }
    }
}

void HungerThirstProcessor::setHungerReceptors(const std::vector<std::shared_ptr<SensoryReceptor>>& receptors) {
    hungerReceptors = receptors;
}

void HungerThirstProcessor::setThirstReceptors(const std::vector<std::shared_ptr<SensoryReceptor>>& receptors) {
    thirstReceptors = receptors;
}

void HungerThirstProcessor::simulateHungerThirstSignals() {
    double hungerLevel = 0.0;
    double thirstLevel = 0.0;

    while (processing.load()) {
        // Increase hunger and thirst levels over time
        hungerLevel = std::min(hungerLevel + 0.01, 1.0);
        thirstLevel = std::min(thirstLevel + 0.01, 1.0);

        processHungerThirstData(hungerLevel, thirstLevel);

        // Sleep to simulate time passing
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void HungerThirstProcessor::processHungerThirstData(double hungerLevel, double thirstLevel) {
    // Stimulate hunger receptors
    for (auto& receptor : hungerReceptors) {
        receptor->stimulate(hungerLevel);
    }

    // Stimulate thirst receptors
    for (auto& receptor : thirstReceptors) {
        receptor->stimulate(thirstLevel);
    }
}
