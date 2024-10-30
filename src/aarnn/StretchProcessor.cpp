// StretchProcessor.cpp

#include "StretchProcessor.h"
#include <chrono>
#include <random>
#include <algorithm>

StretchProcessor::StretchProcessor() : processing(false) {}

StretchProcessor::~StretchProcessor() {
    stopProcessing();
}

bool StretchProcessor::initialise() {
    // Initialize simulation parameters if necessary
    return true;
}

void StretchProcessor::startProcessing() {
    if (!processing.load()) {
        processing = true;
        processingThread = std::thread(&StretchProcessor::simulateStretchDetection, this);
    }
}

void StretchProcessor::stopProcessing() {
    if (processing.load()) {
        processing = false;
        if (processingThread.joinable()) {
            processingThread.join();
        }
    }
}

void StretchProcessor::setStretchReceptors(const std::vector<std::shared_ptr<SensoryReceptor>>& receptors) {
    stretchReceptors = receptors;
}

void StretchProcessor::simulateStretchDetection() {
    // Random number generator for simulating stretch levels
    std::default_random_engine generator;
    std::uniform_real_distribution<double> distribution(0.0, 1.0);

    while (processing.load()) {
        std::vector<double> stretchLevels(stretchReceptors.size());

        // Simulate stretch levels
        for (auto& level : stretchLevels) {
            level = distribution(generator);
        }

        // Process the stretch data
        processStretchData(stretchLevels);

        // Sleep to simulate sampling rate
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

void StretchProcessor::processStretchData(const std::vector<double>& stretchLevels) {
    stimulateReceptors(stretchLevels);
}

void StretchProcessor::stimulateReceptors(const std::vector<double>& levels) {
    for (size_t i = 0; i < stretchReceptors.size(); ++i) {
        double intensity = levels[i];
        stretchReceptors[i]->stimulate(intensity);
    }
}
