// BladderBowelProcessor.cpp

#include "BladderBowelProcessor.h"
#include <chrono>
#include <algorithm>

BladderBowelProcessor::BladderBowelProcessor() : processing(false), bladderLevel(0.0), bowelLevel(0.0) {}

BladderBowelProcessor::~BladderBowelProcessor() {
    stopProcessing();
}

bool BladderBowelProcessor::initialise() {
    return true;
}

void BladderBowelProcessor::startProcessing() {
    if (!processing.load()) {
        processing = true;
        processingThread = std::thread(&BladderBowelProcessor::simulateBladderBowelSensations, this);
    }
}

void BladderBowelProcessor::stopProcessing() {
    if (processing.load()) {
        processing = false;
        if (processingThread.joinable()) {
            processingThread.join();
        }
    }
}

void BladderBowelProcessor::setBladderReceptors(const std::vector<std::shared_ptr<SensoryReceptor>>& receptors) {
    bladderReceptors = receptors;
}

void BladderBowelProcessor::setBowelReceptors(const std::vector<std::shared_ptr<SensoryReceptor>>& receptors) {
    bowelReceptors = receptors;
}

void BladderBowelProcessor::consumeFluid(double amount) {
    bladderLevel = std::min(bladderLevel.load() + amount, 1.0);
}

void BladderBowelProcessor::consumeFood(double amount) {
    bowelLevel = std::min(bowelLevel.load() + amount, 1.0);
}

void BladderBowelProcessor::urinate() {
    bladderLevel = 0.0;
}

void BladderBowelProcessor::defecate() {
    bowelLevel = 0.0;
}

void BladderBowelProcessor::simulateBladderBowelSensations() {
    while (processing.load()) {
        bladderLevel = std::min(bladderLevel.load() + 0.001, 1.0);
        bowelLevel = std::min(bowelLevel.load() + 0.0005, 1.0);

        processBladderBowelData(bladderLevel.load(), bowelLevel.load());

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void BladderBowelProcessor::processBladderBowelData(double bladderLevel, double bowelLevel) {
    // Stimulate bladder receptors
    for (auto& receptor : bladderReceptors) {
        receptor->stimulate(bladderLevel);
    }

    // Stimulate bowel receptors
    for (auto& receptor : bowelReceptors) {
        receptor->stimulate(bowelLevel);
    }
}
