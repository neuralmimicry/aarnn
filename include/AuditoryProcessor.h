// AuditoryProcessor.h
#pragma once

#include <vector>
#include <memory>
#include <atomic>
#include <thread>
#include <string>
#include <chrono>
#include "AsyncNetworkClient.h"
#include "StimuliData.h"
#include "ThreadSafeQueue.h"

// The AuditoryProcessor asynchronously processes audio input,
// transforms it (e.g. via FFT), and sends results via AsyncNetworkClient.
// Each processor has a unique sourceId and is monitored for connection health.
class AuditoryProcessor {
public:
    AuditoryProcessor(const std::string& host, unsigned short port, const std::string& sourceId);
    ~AuditoryProcessor();

    bool initialise();                          // Establish asynchronous connection
    void startProcessing();                     // Begin processing thread
    void stopProcessing();                      // End processing and clean up

    void receiveAudioData(const std::vector<double>& audioData); // Push new audio samples
    bool isHealthy() const;                     // Expose connection health

private:
    std::atomic<bool> processing{false};
    std::atomic<bool> healthy{false};
    std::thread processingThread;

    std::unique_ptr<AsyncNetworkClient> networkClient;
    ThreadSafeQueue<std::vector<double>> audioDataQueue;

    void processAudioDataLoop();
    void performFFTAndSend(const std::vector<double>& audioBuffer);

    static constexpr int FFT_SIZE = 1024;
    std::string sourceId;  // Identifier tag for this processor instance
};

