// PruriceptionProcessor.cpp

#include "PruriceptionProcessor.h"
#include <chrono>
#include <random>
#include <algorithm>

PruriceptionProcessor::PruriceptionProcessor() : processing(false) {}

PruriceptionProcessor::~PruriceptionProcessor() {
    stopProcessing();
}

bool PruriceptionProcessor::initialise() {
    // Initialize simulation parameters if necessary
    return true;
}

void PruriceptionProcessor::startProcessing() {
    if (!processing.load()) {
        processing = true;
        processingThread = std::thread(&PruriceptionProcessor::simulateItchDetection, this);
    }
}

void PruriceptionProcessor::stopProcessing() {
    if (processing.load()) {
        processing = false;
        if (processingThread.joinable()) {
            processingThread.join();
        }
    }
}

void PruriceptionProcessor::setPruriceptiveReceptors(const std::vector<std::shared_ptr<SensoryReceptor>>& receptors) {
    pruriceptiveReceptors = receptors;
}

void PruriceptionProcessor::simulateItchDetection() {
    // Random number generator for simulating itch stimuli
    std::default_random_engine generator;
    std::bernoulli_distribution distribution(0.01); // Low probability of itch event

    while (processing.load()) {
        std::vector<double> itchStimuli(pruriceptiveReceptors.size());

        // Simulate itch stimuli
        for (auto& stimulus : itchStimuli) {
            stimulus = distribution(generator) ? 1.0 : 0.0;
        }

        // Process the itch data
        processItchData(itchStimuli);

        // Sleep to simulate sampling rate
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void PruriceptionProcessor::processItchData(const std::vector<double>& itchStimuli) {
    stimulateReceptors(itchStimuli);
}

void PruriceptionProcessor::stimulateReceptors(const std::vector<double>& stimuli) {
    for (size_t i = 0; i < pruriceptiveReceptors.size(); ++i) {
        double intensity = stimuli[i];
        pruriceptiveReceptors[i]->stimulate(intensity);
    }
}
