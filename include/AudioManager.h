// AudioManager.h
#ifndef AUDIOMANAGER_H
#define AUDIOMANAGER_H

#include <vector>
#include <string>
#include <memory>
#include <thread>
#include <atomic>
#include "PulseAudioMic.h"
#include "ThreadSafeQueue.h" // Assuming you have a thread-safe queue implementation

class AudioManager {
public:
    AudioManager();
    ~AudioManager();

    // Initialize audio components
    bool initialize();

    // Start audio capture
    void startCapture();

    // Stop audio capture
    void stopCapture();

    // Get the audio queue
    ThreadSafeQueue<std::vector<std::tuple<double, double>>>& getAudioQueue();

    // Get the empty audio queue
    ThreadSafeQueue<std::vector<std::tuple<double, double>>>& getEmptyAudioQueue();

private:
    // List ALSA capture devices
    std::vector<std::string> listAlsaCaptureDevices();

    // Automatically select the first ALSA capture device
    std::string autoSelectFirstAlsaCaptureDevice();

    // Audio queues
    ThreadSafeQueue<std::vector<std::tuple<double, double>>> audioQueue;
    ThreadSafeQueue<std::vector<std::tuple<double, double>>> emptyAudioQueue;

    // PulseAudioMic instance
    std::shared_ptr<PulseAudioMic> mic;

    // Capture thread
    std::thread micThread;

    // Selected capture device
    std::string selectedCaptureDevice;

    // Flag to control capture
    std::atomic<bool> capturing;
};

#endif // AUDIOMANAGER_H
