// AuditoryProcessor.cpp
#include "AuditoryProcessor.h"
#include <fftw3.h>
#include <cmath>
#include <iostream>
#include <algorithm>

AuditoryProcessor::AuditoryProcessor(const std::string& host, unsigned short port, const std::string& sourceId)
        : sourceId(sourceId) {
    networkClient = std::make_unique<AsyncNetworkClient>(host, port);
}

AuditoryProcessor::~AuditoryProcessor() {
    stopProcessing();
}

bool AuditoryProcessor::initialise() {
    healthy = networkClient->connect();
    if (!healthy) {
        std::cerr << "AuditoryProcessor: Failed to connect to SensoryReceptor server." << std::endl;
    }
    return healthy;
}

void AuditoryProcessor::startProcessing() {
    if (processing.load()) return;
    processing = true;
    processingThread = std::thread(&AuditoryProcessor::processAudioDataLoop, this);
}

void AuditoryProcessor::stopProcessing() {
    if (!processing.load()) return;
    processing = false;
    if (processingThread.joinable()) {
        processingThread.join();
    }
    networkClient->disconnect();
}

bool AuditoryProcessor::isHealthy() const {
    return healthy.load();
}

void AuditoryProcessor::receiveAudioData(const std::vector<double>& audioData) {
    audioDataQueue.push(audioData);
}

void AuditoryProcessor::processAudioDataLoop() {
    std::vector<double> audioBuffer;
    audioBuffer.reserve(FFT_SIZE);

    while (processing.load()) {
        std::vector<double> audioData;
        if (!audioDataQueue.pop(audioData)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

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

    std::vector<double> magnitudes(FFT_SIZE / 2 + 1);
    for (size_t i = 0; i < magnitudes.size(); ++i) {
        magnitudes[i] = sqrt(out[i][0] * out[i][0] + out[i][1] * out[i][1]);
    }

    double maxMagnitude = *std::max_element(magnitudes.begin(), magnitudes.end());
    if (maxMagnitude > 0) {
        for (auto& mag : magnitudes) {
            mag /= maxMagnitude;
        }
    }

    StimuliData data;
    data.receptorType = "Auditory:" + sourceId;
    data.values = magnitudes;

    std::string serializedData = serializeStimuliData(data);
    try {
        networkClient->send(serializedData);
        healthy = true;
    } catch (...) {
        std::cerr << "AuditoryProcessor: send failed. Marking as unhealthy." << std::endl;
        healthy = false;
    }

    fftw_destroy_plan(p);
    fftw_free(out);
}
