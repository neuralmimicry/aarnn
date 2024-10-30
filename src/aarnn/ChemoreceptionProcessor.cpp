// ChemoreceptionProcessor.cpp

#include "ChemoreceptionProcessor.h"
#include <chrono>
#include <random>
#include <algorithm>

ChemoreceptionProcessor::ChemoreceptionProcessor() : processing(false) {}

ChemoreceptionProcessor::~ChemoreceptionProcessor() {
    stopProcessing();
}

bool ChemoreceptionProcessor::initialise() {
    // Initialize simulation parameters if necessary
    return true;
}

void ChemoreceptionProcessor::startProcessing() {
    if (!processing.load()) {
        processing = true;
        processingThread = std::thread(&ChemoreceptionProcessor::simulateChemicalChanges, this);
    }
}

void ChemoreceptionProcessor::stopProcessing() {
    if (processing.load()) {
        processing = false;
        if (processingThread.joinable()) {
            processingThread.join();
        }
    }
}

void ChemoreceptionProcessor::setChemoreceptiveReceptors(const std::vector<std::shared_ptr<SensoryReceptor>>& receptors) {
    chemoreceptiveReceptors = receptors;
}

void ChemoreceptionProcessor::simulateChemicalChanges() {
    // Random number generator for simulating chemical levels
    std::default_random_engine generator;
    std::normal_distribution<double> distribution(0.5, 0.1); // Mean level

    while (processing.load()) {
        std::vector<double> chemicalLevels(chemoreceptiveReceptors.size());

        // Simulate chemical levels
        for (auto& level : chemicalLevels) {
            level = distribution(generator);
            level = std::clamp(level, 0.0, 1.0);
        }

        // Process the chemical data
        processChemicalData(chemicalLevels);

        // Sleep to simulate sampling rate
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void ChemoreceptionProcessor::processChemicalData(const std::vector<double>& chemicalLevels) {
    stimulateReceptors(chemicalLevels);
}

void ChemoreceptionProcessor::stimulateReceptors(const std::vector<double>& levels) {
    for (size_t i = 0; i < chemoreceptiveReceptors.size(); ++i) {
        double intensity = levels[i];
        chemoreceptiveReceptors[i]->stimulate(intensity);
    }
}
