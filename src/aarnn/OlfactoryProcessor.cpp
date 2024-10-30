// OlfactoryProcessor.cpp

#include "OlfactoryProcessor.h"

OlfactoryProcessor::OlfactoryProcessor() : processing(false) {}

OlfactoryProcessor::~OlfactoryProcessor() {
    stopProcessing();
}

bool OlfactoryProcessor::initialise() {
    // Initialize sensors or simulation parameters
    return true;
}

void OlfactoryProcessor::startProcessing() {
    if (!processing.load()) {
        processing = true;
        processingThread = std::thread(&OlfactoryProcessor::simulateChemicalDetection, this);
    }
}

void OlfactoryProcessor::stopProcessing() {
    if (processing.load()) {
        processing = false;
        if (processingThread.joinable()) {
            processingThread.join();
        }
    }
}

void OlfactoryProcessor::setOlfactoryReceptors(const std::vector<std::shared_ptr<SensoryReceptor>>& receptors) {
    olfactoryReceptors = receptors;
}

void OlfactoryProcessor::simulateChemicalDetection() {
    while (processing.load()) {
        // Simulate detection of chemicals
        // For testing, generate random concentrations
        std::vector<double> chemicalConcentrations(olfactoryReceptors.size());
        for (auto& concentration : chemicalConcentrations) {
            concentration = /* simulate concentration */;
        }

        // Process the chemical data
        processChemicalData(chemicalConcentrations);

        // Sleep to simulate sampling rate
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void OlfactoryProcessor::processChemicalData(const std::vector<double>& chemicalConcentrations) {
    // Map chemical concentrations to receptors
    stimulateReceptors(chemicalConcentrations);
}

void OlfactoryProcessor::stimulateReceptors(const std::vector<double>& concentrations) {
    for (size_t i = 0; i < olfactoryReceptors.size(); ++i) {
        double intensity = concentrations[i];
        olfactoryReceptors[i]->stimulate(intensity);
    }
}
