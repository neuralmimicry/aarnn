// PressureProcessor.cpp

#include "PressureProcessor.h"
#include <chrono>
#include <random>
#include <algorithm>

PressureProcessor::PressureProcessor() : processing(false) {}

PressureProcessor::~PressureProcessor() {
    stopProcessing();
}

bool PressureProcessor::initialise() {
    // Initialize simulation parameters if necessary
    return true;
}

void PressureProcessor::startProcessing() {
    if (!processing.load()) {
        processing = true;
        processingThread = std::thread(&PressureProcessor::simulatePressureChanges, this);
    }
}

void PressureProcessor::stopProcessing() {
    if (processing.load()) {
        processing = false;
        if (processingThread.joinable()) {
            processingThread.join();
        }
    }
}

void PressureProcessor::setPressureReceptors(const std::vector<std::shared_ptr<SensoryReceptor>>& receptors) {
    pressureReceptors = receptors;
}

void PressureProcessor::simulatePressureChanges() {
    // Random number generator for simulating pressure changes
    std::default_random_engine generator;
    std::normal_distribution<double> distribution(0.5, 0.1); // Mean pressure level

    while (processing.load()) {
        std::vector<double> pressureLevels(pressureReceptors.size());

        // Simulate pressure levels
        for (auto& level : pressureLevels) {
            level = distribution(generator);
            level = std::clamp(level, 0.0, 1.0);
        }

        // Process the pressure data
        processPressureData(pressureLevels);

        // Sleep to simulate sampling rate
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

void PressureProcessor::processPressureData(const std::vector<double>& pressureLevels) {
    stimulateReceptors(pressureLevels);
}

void PressureProcessor::stimulateReceptors(const std::vector<double>& levels) {
    for (size_t i = 0; i < pressureReceptors.size(); ++i) {
        double intensity = levels[i];
        pressureReceptors[i]->stimulate(intensity);
    }
}
