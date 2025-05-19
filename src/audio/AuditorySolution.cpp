#include "AuditoryManager.h"
#include "AuditoryProcessor.h"

int main() {
    // Create AuditoryProcessor
    std::string host = "sensory_receptor_host";
    unsigned short port = 12345;
    AuditoryProcessor auditoryProcessor(host, port, "auditory_processor_1");
    if (!auditoryProcessor.initialise()) {
        std::cerr << "Failed to initialise AuditoryProcessor." << std::endl;
        return -1;
    }
    auditoryProcessor.startProcessing();

    // Create AuditoryManager
    AuditoryManager auditoryManager;
    if (!auditoryManager.initialise()) {
        std::cerr << "Failed to initialise AuditoryManager." << std::endl;
        return -1;
    }

    // Select YouTube input
    std::string apiKey = "YOUR_YOUTUBE_API_KEY";
    std::string searchQuery = "Learning to read for kids, in English (UK)";
    if (!auditoryManager.selectYouTubeInput(apiKey, searchQuery)) {
        std::cerr << "Failed to select YouTube input." << std::endl;
        return -1;
    }

    // Select RTSP input
    std::string rtspUrl = "rtsp://your_rtsp_stream_url";
    if (!auditoryManager.selectRTSPInput(rtspUrl)) {
        std::cerr << "Failed to select RTSP input." << std::endl;
        return -1;
    }

    // Select input source
    // For microphone input
    if (!auditoryManager.selectMicrophoneInput()) {
        std::cerr << "Failed to select microphone input." << std::endl;
        return -1;
    }

    // For audio file input
    /*
    std::string audioFilePath = "path/to/audiofile.wav";
    if (!auditoryManager.selectFileInput(audioFilePath)) {
        std::cerr << "Failed to select audio file input." << std::endl;
        return -1;
    }
    */

    // Set the audio data callback
    auditoryManager.setAudioDataCallback([&auditoryProcessor](const std::vector<double>& audioData) {
        auditoryProcessor.receiveAudioData(audioData);
    });

    // Start capturing audio
    auditoryManager.startCapture();

    // Main loop or wait
    // ...

    // Clean up
    auditoryManager.stopCapture();
    auditoryProcessor.stopProcessing();

    return 0;
}
