// HeartbeatRespirationProcessor.cpp

#include "HeartbeatRespirationProcessor.h"
#include <chrono>
#include <cmath>

HeartbeatRespirationProcessor::HeartbeatRespirationProcessor() : processing(false) {}

HeartbeatRespirationProcessor::~HeartbeatRespirationProcessor() {
    stopProcessing();
}

bool HeartbeatRespirationProcessor::initialise() {
    return true;
}

void HeartbeatRespirationProcessor::startProcessing() {
    if (!processing.load()) {
        processing = true;
        processingThread = std::thread(&HeartbeatRespirationProcessor::simulateHeartbeatRespiration, this);
    }
}

void HeartbeatRespirationProcessor::stopProcessing() {
    if (processing.load()) {
        processing = false;
        if (processingThread.joinable()) {
            processingThread.join();
        }
    }
}

void HeartbeatRespirationProcessor::setHeartbeatReceptors(const std::vector<std::shared_ptr<SensoryReceptor>>& receptors) {
    heartbeatReceptors = receptors;
}

void HeartbeatRespirationProcessor::setRespirationReceptors(const std::vector<std::shared_ptr<SensoryReceptor>>& receptors) {
    respirationReceptors = receptors;
}

void HeartbeatRespirationProcessor::simulateHeartbeatRespiration() {
    double time = 0.0;
    const double heartbeatFrequency = 1.0; // 1 Hz (60 BPM)
    const double respirationFrequency = 0.2; // 0.2 Hz (12 breaths per minute)

    while (processing.load()) {
        double heartbeatSignal = 0.5 * (1.0 + std::sin(2 * M_PI * heartbeatFrequency * time));
        double respirationSignal = 0.5 * (1.0 + std::sin(2 * M_PI * respirationFrequency * time));

        processHeartbeatRespirationData(heartbeatSignal, respirationSignal);

        time += 0.1; // Increment time
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void HeartbeatRespirationProcessor::processHeartbeatRespirationData(double heartbeatSignal, double respirationSignal) {
    // Stimulate heartbeat receptors
    for (auto& receptor : heartbeatReceptors) {
        receptor->stimulate(heartbeatSignal);
    }

    // Stimulate respiration receptors
    for (auto& receptor : respirationReceptors) {
        receptor->stimulate(respirationSignal);
    }
}
