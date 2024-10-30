// LustProcessor.cpp

#include "LustProcessor.h"
#include <chrono>
#include <algorithm>

LustProcessor::LustProcessor() : processing(false), hormoneLevel(0.5) {}

LustProcessor::~LustProcessor() {
    stopProcessing();
}

bool LustProcessor::initialise() {
    return true;
}

void LustProcessor::startProcessing() {
    if (!processing.load()) {
        processing = true;
        processingThread = std::thread(&LustProcessor::simulateLustSignals, this);
    }
}

void LustProcessor::stopProcessing() {
    if (processing.load()) {
        processing = false;
        if (processingThread.joinable()) {
            processingThread.join();
        }
    }
}

void LustProcessor::setLustReceptors(const std::vector<std::shared_ptr<SensoryReceptor>>& receptors) {
    lustReceptors = receptors;
}

void LustProcessor::adjustHormoneLevels(double amount) {
    hormoneLevel = std::clamp(hormoneLevel.load() + amount, 0.0, 1.0);
}

void LustProcessor::simulateLustSignals() {
    while (processing.load()) {
        hormoneLevel = std::clamp(hormoneLevel.load() + 0.001, 0.0, 1.0);

        processLustData(hormoneLevel.load());

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void LustProcessor::processLustData(double level) {
    for (auto& receptor : lustReceptors) {
        receptor->stimulate(level);
    }
}
