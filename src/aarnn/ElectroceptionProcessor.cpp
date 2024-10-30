// ElectroreceptionProcessor.cpp

#include "ElectroreceptionProcessor.h"
#include <chrono>
#include <random>
#include <algorithm>

ElectroreceptionProcessor::ElectroreceptionProcessor() : processing(false) {}

ElectroreceptionProcessor::~ElectroreceptionProcessor() {
    stopProcessing();
}

bool ElectroreceptionProcessor::initialise() {
    // Initialize simulation parameters if necessary
    return true;
}

void ElectroreceptionProcessor::startProcessing() {
    if (!processing.load()) {
        processing = true;
        processingThread = std::thread(&ElectroreceptionProcessor::simulateElectricFieldDetection, this);
    }
}

void ElectroreceptionProcessor::stopProcessing() {
    if (processing.load()) {
        processing = false;
        if (processingThread.joinable()) {
            processingThread.join();
        }
    }
}

void ElectroreceptionProcessor::setElectroreceptiveReceptors(const std::vector<std::shared_ptr<SensoryReceptor>>& receptors) {
    electroreceptiveReceptors = receptors;
}

void ElectroreceptionProcessor::simulateElectricFieldDetection() {
    // Random number generator for simulating electric field intensity
    std::default_random_engine generator;
    std::uniform_real_distribution<double> distribution(0.0, 1.0);

    while (processing.load()) {
        std::vector<double> electricFieldIntensities(electroreceptiveReceptors.size());

        // Simulate electric field intensities
        for (auto& intensity : electricFieldIntensities) {
            intensity = distribution(generator);
        }

        // Process the electric field data
        processElectricFieldData(electricFieldIntensities);

        // Sleep to simulate sampling rate
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void ElectroreceptionProcessor::processElectricFieldData(const std::vector<double>& electricFieldIntensities) {
    stimulateReceptors(electricFieldIntensities);
}

void ElectroreceptionProcessor::stimulateReceptors(const std::vector<double>& intensities) {
    for (size_t i = 0; i < electroreceptiveReceptors.size(); ++i) {
        double intensity = intensities[i];
        electroreceptiveReceptors[i]->stimulate(intensity);
    }
}
