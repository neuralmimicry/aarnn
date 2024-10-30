// SatietyProcessor.cpp

#include "SatietyProcessor.h"
#include <chrono>
#include <algorithm>

SatietyProcessor::SatietyProcessor() : processing(false), satietyLevel(0.0) {}

SatietyProcessor::~SatietyProcessor() {
    stopProcessing();
}

bool SatietyProcessor::initialise() {
    return true;
}

void SatietyProcessor::startProcessing() {
    if (!processing.load()) {
        processing = true;
        processingThread = std::thread(&SatietyProcessor::simulateSatietySignals, this);
    }
}

void SatietyProcessor::stopProcessing() {
    if (processing.load()) {
        processing = false;
        if (processingThread.joinable()) {
            processingThread.join();
        }
    }
}

void SatietyProcessor::setSatietyReceptors(const std::vector<std::shared_ptr<SensoryReceptor>>& receptors) {
    satietyReceptors = receptors;
}

void SatietyProcessor::consumeFood(double amount) {
    // Increase satiety level based on food amount
    satietyLevel = std::min(satietyLevel.load() + amount, 1.0);
}

void SatietyProcessor::simulateSatietySignals() {
    while (processing.load()) {
        // Decrease satiety level over time
        satietyLevel = std::max(satietyLevel.load() - 0.005, 0.0);

        processSatietyData(satietyLevel.load());

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void SatietyProcessor::processSatietyData(double level) {
    for (auto& receptor : satietyReceptors) {
        receptor->stimulate(level);
    }
}
