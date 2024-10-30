// HeartbeatRespirationProcessor.h

#pragma once

#include <vector>
#include <memory>
#include <atomic>
#include <thread>
#include "SensoryReceptor.h"

class HeartbeatRespirationProcessor {
public:
    HeartbeatRespirationProcessor();
    ~HeartbeatRespirationProcessor();

    bool initialise();
    void startProcessing();
    void stopProcessing();

    void setHeartbeatReceptors(const std::vector<std::shared_ptr<SensoryReceptor>>& receptors);
    void setRespirationReceptors(const std::vector<std::shared_ptr<SensoryReceptor>>& receptors);

private:
    std::vector<std::shared_ptr<SensoryReceptor>> heartbeatReceptors;
    std::vector<std::shared_ptr<SensoryReceptor>> respirationReceptors;

    // Thread management
    std::atomic<bool> processing;
    std::thread processingThread;

    // Internal methods
    void simulateHeartbeatRespiration();
    void processHeartbeatRespirationData(double heartbeatSignal, double respirationSignal);
};
