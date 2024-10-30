// GustatoryProcessor.cpp

#include "GustatoryProcessor.h"

GustatoryProcessor::GustatoryProcessor() : processing(false) {}

GustatoryProcessor::~GustatoryProcessor() {
    stopProcessing();
}

bool GustatoryProcessor::initialise() {
    // Initialize sensors or simulation parameters
    return true;
}

void GustatoryProcessor::startProcessing() {
    if (!processing.load()) {
        processing = true;
        processingThread = std::thread(&GustatoryProcessor::simulateTasteDetection, this);
    }
}

void GustatoryProcessor::stopProcessing() {
    if (processing.load()) {
        processing = false;
        if (processingThread.joinable()) {
            processingThread.join();
        }
    }
}

void GustatoryProcessor::setGustatoryReceptors(const std::vector<std::shared_ptr<SensoryReceptor>>& receptors) {
    gustatoryReceptors = receptors;
}

void GustatoryProcessor::simulateTasteDetection() {
    while (processing.load()) {
        // Simulate detection of taste stimuli
        std::vector<double> tasteIntensities(gustatoryReceptors.size());
        for (auto& intensity : tasteIntensities) {
            intensity = /* simulate intensity */;
        }

        // Process the taste data
        processTasteData(tasteIntensities);

        // Sleep to simulate sampling rate
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void GustatoryProcessor::processTasteData(const std::vector<double>& tasteIntensities) {
    // Map taste intensities to receptors
    stimulateReceptors(tasteIntensities);
}

void GustatoryProcessor::stimulateReceptors(const std::vector<double>& intensities) {
    for (size_t i = 0; i < gustatoryReceptors.size(); ++i) {
        double intensity = intensities[i];
        gustatoryReceptors[i]->stimulate(intensity);
    }
}
