// AuditoryManager.h
#ifndef AUDITORYMANAGER_H
#define AUDITORYMANAGER_H

#include <vector>
#include <string>
#include <memory>
#include <thread>
#include <atomic>
#include "PulseAudioMic.h"
#include "ThreadSafeQueue.h"
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
    ThreadSafeQueue<std::vector<std::tuple<double, double>>>& getEmptyAuditoryQueue(); // ✅ Added missing declaration

    std::vector<std::string> listAlsaCaptureDevices();
    std::string autoSelectFirstAlsaCaptureDevice(); // ✅ Added missing declaration

private:
    std::shared_ptr<PulseAudioMic> mic;
    std::thread micThread;
    std::atomic<bool> capturing;

    std::atomic<bool> processing;
    std::thread processingThread;
    std::vector<std::shared_ptr<SensoryReceptor>> sensoryReceptors;

    ThreadSafeQueue<std::vector<std::tuple<double, double>>> audioQueue;
    ThreadSafeQueue<std::vector<std::tuple<double, double>>> emptyAuditoryQueue; // ✅ Ensure it exists in class scope

    void processAuditoryData();
    void performFFTAndStimulate(const std::vector<double>& audioBuffer);
};

#endif // AUDITORYMANAGER_H
