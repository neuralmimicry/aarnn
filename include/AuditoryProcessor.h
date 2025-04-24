// AuditoryProcessor.h

#pragma once

#include <vector>
#include <memory>
#include <atomic>
#include <thread>
#include <string>
#include "NetworkClient.h"
#include "StimuliData.h"
#include "ThreadSafeQueue.h"

class AuditoryProcessor {
public:
    AuditoryProcessor(const std::string& host, unsigned short port);
    ~AuditoryProcessor();

    bool initialise();
    void startProcessing();
    void stopProcessing();

    // Method to receive audio data from AuditoryManager
    void receiveAudioData(const std::vector<double>& audioData);

private:
    std::atomic<bool> processing{false};
    std::thread processingThread;

    std::unique_ptr<NetworkClient> networkClient;

    // Internal methods
    void processAudioDataLoop();
    void performFFTAndSend(const std::vector<double>& audioBuffer);

    // Thread-safe queue for audio data
    ThreadSafeQueue<std::vector<double>> audioDataQueue;

    // Constants
    static constexpr int FFT_SIZE = 1024; // Adjust as needed
};
