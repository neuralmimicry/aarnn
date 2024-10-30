// MagnetoceptionProcessor.cpp

#include "MagnetoceptionProcessor.h"
#include <chrono>
#include <random>
#include <algorithm>

MagnetoceptionProcessor::MagnetoceptionProcessor() : processing(false) {}

MagnetoceptionProcessor::~MagnetoceptionProcessor() {
    stopProcessing();
}

bool MagnetoceptionProcessor::initialise() {
    // Initialize simulation parameters if necessary
    return true;
}

void MagnetoceptionProcessor::startProcessing() {
    if (!processing.load()) {
        processing = true;
        processingThread = std::thread(&MagnetoceptionProcessor::simulateMagneticFieldDetection, this);
    }
}

void MagnetoceptionProcessor::stopProcessing() {
    if (processing.load()) {
        processing = false;
        if (processingThread.joinable()) {
            processingThread.join();
        }
    }
}

void MagnetoceptionProcessor::setMagnetoceptiveReceptors(const std::vector<std::shared_ptr<SensoryReceptor>>& receptors) {
    magnetoceptiveReceptors = receptors;
}

void MagnetoceptionProcessor::simulateMagneticFieldDetection() {
    // Random number generator for simulating magnetic field strength
    std::default_random_engine generator;
    std::uniform_real_distribution<double> distribution(0.0, 1.0);

    while (processing.load()) {
        std::vector<double> magneticFieldStrengths(magnetoceptiveReceptors.size());

        // Simulate magnetic field strengths
        for (auto& strength : magneticFieldStrengths) {
            strength = distribution(generator);
        }

        // Process the magnetic field data
        processMagneticFieldData(magneticFieldStrengths);

        // Sleep to simulate sampling rate
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void MagnetoceptionProcessor::processMagneticFieldData(const std::vector<double>& magneticFieldStrengths) {
    stimulateReceptors(magneticFieldStrengths);
}

void MagnetoceptionProcessor::stimulateReceptors(const std::vector<double>& strengths) {
    for (size_t i = 0; i < magnetoceptiveReceptors.size(); ++i) {
        double intensity = strengths[i];
        magnetoceptiveReceptors[i]->stimulate(intensity);
    }
}
