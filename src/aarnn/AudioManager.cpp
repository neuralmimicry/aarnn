// AuditoryManager.cpp
#include "AuditoryManager.h"
#include <iostream>
#include <alsa/asoundlib.h> // Ensure you have ALSA development libraries
#include <cstring> // For std::to_string

AuditoryManager::AuditoryManager() : capturing(false) {}

AuditoryManager::~AuditoryManager() {
    stopCapture();
}

bool AuditoryManager::initialise() {
    selectedCaptureDevice = autoSelectFirstAlsaCaptureDevice();
    if (selectedCaptureDevice.empty()) {
        std::cerr << "Failed to select an ALSA capture device." << std::endl;
        return false;
    }

    mic = std::make_shared<PulseAuditoryMic>(audioQueue);
    if (!mic) {
        std::cerr << "Failed to create PulseAuditoryMic!" << std::endl;
        return false;
    }

    return true;
}

void AuditoryManager::startCapture() {
    if (mic && !capturing.load()) {
        capturing = true;
        micThread = std::thread(&PulseAuditoryMic::micRun, mic);
    }
}

void AuditoryManager::stopCapture() {
    if (mic && capturing.load()) {
        capturing = false;
        mic->micStop();
        if (micThread.joinable()) {
            micThread.join();
        }
    }
}

ThreadSafeQueue<std::vector<std::tuple<double, double>>>& AuditoryManager::getAuditoryQueue() {
    return audioQueue;
}

ThreadSafeQueue<std::vector<std::tuple<double, double>>>& AuditoryManager::getEmptyAuditoryQueue() {
    return emptyAuditoryQueue;
}

std::vector<std::string> AuditoryManager::listAlsaCaptureDevices() {
    std::vector<std::string> captureDevices;
    int card = -1;
    int err;

    // Iterate through sound cards
    while ((err = snd_card_next(&card)) >= 0 && card >= 0) {
        char* cardName = nullptr;
        if ((err = snd_card_get_name(card, &cardName)) < 0) {
            std::cerr << "Cannot get name for card " << card << ": " << snd_strerror(err) << std::endl;
            continue;
        }

        // Open control interface for the card
        snd_ctl_t* ctl;
        std::string ctlName = "hw:" + std::to_string(card);
        if ((err = snd_ctl_open(&ctl, ctlName.c_str(), 0)) < 0) {
            std::cerr << "Cannot open control for card " << card << ": " << snd_strerror(err) << std::endl;
            free(cardName);
            continue;
        }

        int device = -1;
        // Iterate through PCM devices on this card
        while ((err = snd_ctl_pcm_next_device(ctl, &device)) >= 0 && device >= 0) {
            snd_pcm_info_t* pcmInfo;
            snd_pcm_info_alloca(&pcmInfo);
            snd_pcm_info_set_device(pcmInfo, device);
            snd_pcm_info_set_subdevice(pcmInfo, 0);
            snd_pcm_info_set_stream(pcmInfo, SND_PCM_STREAM_CAPTURE);

            if ((err = snd_ctl_pcm_info(ctl, pcmInfo)) >= 0) {
                const char* name = snd_pcm_info_get_name(pcmInfo);
                // Construct device identifier (e.g., "hw:0,0")
                std::string deviceId = "hw:" + std::to_string(card) + "," + std::to_string(device);
                captureDevices.emplace_back(deviceId);
                std::cout << "Found capture device: " << deviceId << " (" << name << ")" << std::endl;
            }
        }

        if (err < 0 && err != -ENOENT) {
            std::cerr << "Error iterating PCM devices for card " << card << ": " << snd_strerror(err) << std::endl;
        }

        snd_ctl_close(ctl);
        free(cardName);
    }

    if (err < 0 && err != -ENOENT) {
        std::cerr << "Error iterating sound cards: " << snd_strerror(err) << std::endl;
    }

    return captureDevices;
}

std::string AuditoryManager::autoSelectFirstAlsaCaptureDevice() {
    std::vector<std::string> devices = listAlsaCaptureDevices();

    char* pulse_sink = std::getenv("PULSE_SINK");
    if (pulse_sink != nullptr) {
        std::cout << "PULSE_SINK: " << pulse_sink << std::endl;
    } else {
        std::cout << "PULSE_SINK not set" << std::endl;
    }

    char* pulse_source = std::getenv("PULSE_SOURCE");
    if (pulse_source != nullptr) {
        std::cout << "PULSE_SOURCE: " << pulse_source << std::endl;
    } else {
        std::cout << "PULSE_SOURCE not set" << std::endl;
    }

    ThreadSafeQueue<std::vector<std::tuple<double, double>>> audioQueue;
    ThreadSafeQueue<std::vector<std::tuple<double, double>>> emptyAuditoryQueue;

    std::shared_ptr<PulseAuditoryMic> mic = std::make_shared<PulseAuditoryMic>(audioQueue);
    if (!mic) {
        std::cerr << "Failed to create PulseAuditoryMic!" << std::endl;
        return "";
    }
    std::thread micThread(&PulseAuditoryMic::micRun, mic);

    if (devices.empty()) {
        std::cerr << "Error: No ALSA capture devices found." << std::endl;
        return "";
    }
    std::cout << "Automatically selected ALSA capture device: " << devices[0] << std::endl;
    return devices[0];
}

void AuditoryManager::setSensoryReceptors(const std::vector<std::shared_ptr<SensoryReceptor>>& receptors) {
    sensoryReceptors = receptors;
}

void AuditoryManager::startProcessing() {
    if (!processing.load()) {
        processing = true;
        processingThread = std::thread(&AuditoryManager::processAuditoryData, this);
    }
}

void AuditoryManager::stopProcessing() {
    if (processing.load()) {
        processing = false;
        if (processingThread.joinable()) {
            processingThread.join();
        }
    }
}

void AuditoryManager::processAuditoryData() {
    const int FFT_SIZE = 1024; // Adjust as needed
    std::vector<double> audioBuffer;
    audioBuffer.reserve(FFT_SIZE);

    while (processing.load()) {
        std::vector<std::tuple<double, double>> audioData;
        if (audioQueue.pop(audioData)) {
            // Extract the audio samples (assuming mono audio)
            for (const auto& sample : audioData) {
                double leftSample = std::get<0>(sample);
                audioBuffer.push_back(leftSample);
                if (audioBuffer.size() >= FFT_SIZE) {
                    // Perform FFT on audioBuffer
                    performFFTAndStimulate(audioBuffer);
                    // Clear buffer for next batch
                    audioBuffer.clear();
                }
            }
        } else {
            // Sleep briefly to avoid busy-waiting
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

void AuditoryManager::performFFTAndStimulate(const std::vector<double>& audioBuffer) {
    const int FFT_SIZE = audioBuffer.size();
    fftw_complex* out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * (FFT_SIZE / 2 + 1));
    fftw_plan p = fftw_plan_dft_r2c_1d(FFT_SIZE, const_cast<double*>(audioBuffer.data()), out, FFTW_ESTIMATE);
    fftw_execute(p);

    // Stimulate SensoryReceptors based on frequency amplitudes
    for (size_t i = 0; i < sensoryReceptors.size(); ++i) {
        if (i < static_cast<size_t>(FFT_SIZE / 2 + 1)) {
            double magnitude = sqrt(out[i][0] * out[i][0] + out[i][1] * out[i][1]);
            sensoryReceptors[i]->stimulate(magnitude);
        }
    }

    fftw_destroy_plan(p);
    fftw_free(out);
}
