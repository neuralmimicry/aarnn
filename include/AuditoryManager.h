// AuditoryManager.h
#ifndef AUDIOMANAGER_H
#define AUDIOMANAGER_H

#include <vector>
#include <string>
#include <memory>
#include <thread>
#include <atomic>
#include "PulseAudioMic.h"
#include "ThreadSafeQueue.h" // Assuming you have a thread-safe queue implementation
#include "SensoryReceptor.h"

class AuditoryManager {
public:
    AuditoryManager();
    ~AuditoryManager();

    bool initialise();
    void startCapture();
    void stopCapture();

    void setSensoryReceptors(const std::vector<std::shared_ptr<SensoryReceptor>>& receptors);
    void startProcessing();
    void stopProcessing();

    ThreadSafeQueue<std::vector<std::tuple<double, double>>>& getAuditoryQueue();

private:
    std::shared_ptr<PulseAudioMic> mic;
    std::thread micThread;
    std::atomic<bool> capturing;

    // New members for processing
    std::atomic<bool> processing;
    std::thread processingThread;
    std::vector<std::shared_ptr<SensoryReceptor>> sensoryReceptors;

    ThreadSafeQueue<std::vector<std::tuple<double, double>>> audioQueue;

    void processAuditoryData();
    void performFFTAndStimulate(const std::vector<double>& audioBuffer);
};

#endif // AUDIOMANAGER_H
