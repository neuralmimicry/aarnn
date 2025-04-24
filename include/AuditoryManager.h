#pragma once

#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <thread>
#include <functional>
#include <tuple>
#include "PulseAudioMic.h"
#include "ThreadSafeQueue.h"
#include "SensoryReceptor.h"

class AuditoryManager {
public:
    AuditoryManager();
    ~AuditoryManager();

    bool initialise();

    // Input source selection methods
    bool selectMicrophoneInput();
    bool selectFileInput(const std::string& filePath);
    bool selectRTSPInput(const std::string& rtspUrl);
    bool selectYouTubeInput(const std::string& apiKey, const std::string& searchQuery);

    void startCapture();
    void stopCapture();

    // Method to set the callback function for audio data
    void setAudioDataCallback(const std::function<void(const std::vector<double>&)>& callback);

    // Processing control
    void setSensoryReceptors(const std::vector<std::shared_ptr<SensoryReceptor>>& receptors);
    void startProcessing();
    void stopProcessing();

private:
    // Input source types
    enum class InputSource {
        None,
        Microphone,
        AudioFile,
        RTSPStream,
        YouTubeStream
    };

    InputSource inputSource{InputSource::None};

    // Selected ALSA capture device identifier
    std::string selectedCaptureDevice;

    // Queue for raw audio data tuples (left, right)
    ThreadSafeQueue<std::vector<std::tuple<double, double>>> audioQueue;

    // Microphone input variables
    std::shared_ptr<PulseAudioMic> mic;
    std::thread micThread;
    std::atomic<bool> capturing{false};

    // Audio file input variables
    std::string audioFilePath;
    std::thread fileThread;

    // RTSP input variables
    std::string rtspUrl;
    std::thread rtspThread;

    // YouTube input variables
    std::string youTubeApiKey;
    std::string youTubeSearchQuery;
    std::string youTubeVideoUrl;
    std::thread youTubeThread;

    // Audio data callback
    std::function<void(const std::vector<double>&)> audioDataCallback;

    // Processing variables
    std::vector<std::shared_ptr<SensoryReceptor>> sensoryReceptors;
    std::atomic<bool> processing{false};
    std::thread processingThread;

    // Internal capture methods
    void captureFromMicrophone();
    void captureFromFile();
    void captureFromRTSP();
    void captureFromYouTube();

    // Processing worker functions
    void processAuditoryData();
    void performFFTAndStimulate(const std::vector<double>& audioBuffer);

    // Helper methods
    std::vector<std::string> listAlsaCaptureDevices();
    std::string autoSelectFirstAlsaCaptureDevice();
    std::string searchYouTubeVideo(const std::string& apiKey, const std::string& searchQuery);
};
