// InteroceptiveProcessor.cpp

#include "InteroceptiveProcessor.h"

InteroceptiveProcessor::InteroceptiveProcessor() : processing(false) {}

InteroceptiveProcessor::~InteroceptiveProcessor() {
    stopProcessing();
}

bool InteroceptiveProcessor::initialise() {
    // Initialize simulation parameters
    return true;
}

void InteroceptiveProcessor::startProcessing() {
    if (!processing.load()) {
        processing = true;
        processingThread = std::thread(&InteroceptiveProcessor::simulateInteroceptiveInput, this);
    }
}

void InteroceptiveProcessor::stopProcessing() {
    if (processing.load()) {
        processing = false;
        if (processingThread.joinable()) {
            processingThread.join();
        }
    }
}

void InteroceptiveProcessor::setInteroceptiveReceptors(const std::vector<std::shared_ptr<SensoryReceptor>>& receptors) {
    interoceptiveReceptors = receptors;
}

void InteroceptiveProcessor::simulateInteroceptiveInput() {
    while (processing.load()) {
        // Simulate internal states
        std::vector<double> internalStimuli(interoceptiveReceptors.size());

        // Generate stimuli
        for (auto& stimulus : internalStimuli) {
            stimulus = /* simulate internal stimulus */;
        }

        // Process the data
        processInteroceptiveData(internalStimuli);

        // Sleep to simulate sampling rate
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void InteroceptiveProcessor::processInteroceptiveData(const std::vector<double>& internalStimuli) {
    // Stimulate interoceptive receptors
    for (size_t i = 0; i < interoceptiveReceptors.size(); ++i) {
        interoceptiveReceptors[i]->stimulate(internalStimuli[i]);
    }
}
