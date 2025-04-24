// AuditoryProcessor.cpp

#include "AuditoryProcessor.h"
#include <fftw3.h>
#include <cmath>
#include <iostream>

AuditoryProcessor::AuditoryProcessor(const std::string& host, unsigned short port)
        : processing(false)
{
    networkClient = std::make_unique<NetworkClient>(host, port);
}

AuditoryProcessor::~AuditoryProcessor() {
    stopProcessing();
}

bool AuditoryProcessor::initialise() {
    if (!networkClient->connect()) {
        std::cerr << "Failed to connect to SensoryReceptor server." << std::endl;
        return false;
    }
    return true;
}

void AuditoryProcessor::startProcessing() {
    if (processing.load()) {
        return;
    }
    processing = true;
    processingThread = std::thread(&AuditoryProcessor::processAudioDataLoop, this);
}

void AuditoryProcessor::stopProcessing() {
    if (!processing.load()) {
        return;
    }
    processing = false;
    if (processingThread.joinable()) {
        processingThread.join();
    }
}

void AuditoryProcessor::receiveAudioData(const std::vector<double>& audioData) {
    audioDataQueue.push(audioData);
}

void AuditoryProcessor::processAudioDataLoop() {
    std::vector<double> audioBuffer;
    audioBuffer.reserve(FFT_SIZE);

    while (processing.load()) {
        auto audioData = audioDataQueue.pop();
        audioBuffer.insert(audioBuffer.end(), audioData.begin(), audioData.end());
        while (audioBuffer.size() >= FFT_SIZE) {
            std::vector<double> fftInput(audioBuffer.begin(), audioBuffer.begin() + FFT_SIZE);
            performFFTAndSend(fftInput);
            audioBuffer.erase(audioBuffer.begin(), audioBuffer.begin() + FFT_SIZE);
        }
    }
}

void AuditoryProcessor::performFFTAndSend(const std::vector<double>& audioBuffer) {
    fftw_complex* out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * (FFT_SIZE / 2 + 1));
    fftw_plan p = fftw_plan_dft_r2c_1d(FFT_SIZE, const_cast<double*>(audioBuffer.data()), out, FFTW_ESTIMATE);
    fftw_execute(p);

    // Prepare frequency magnitudes
    std::vector<double> magnitudes(FFT_SIZE / 2 + 1);
    for (size_t i = 0; i < magnitudes.size(); ++i) {
        magnitudes[i] = sqrt(out[i][0] * out[i][0] + out[i][1] * out[i][1]);
    }

    // Normalize magnitudes
    double maxMagnitude = *std::max_element(magnitudes.begin(), magnitudes.end());
    if (maxMagnitude > 0) {
        for (auto& mag : magnitudes) {
            mag /= maxMagnitude;
        }
    }

    // Prepare stimuli data
    StimuliData data;
    data.receptorType = "Auditory";
    data.values = magnitudes;

    // Serialize the data
    std::string serializedData = serializeStimuliData(data);

    // Send data using the NetworkClient
    if (!networkClient->sendData(serializedData)) {
        std::cerr << "Failed to send data to SensoryReceptor server." << std::endl;
        // Handle error (e.g., attempt to reconnect)
    }

    fftw_destroy_plan(p);
    fftw_free(out);
}
