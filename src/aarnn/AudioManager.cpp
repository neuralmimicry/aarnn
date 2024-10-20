// AudioManager.cpp
#include "AudioManager.h"
#include <iostream>
#include <alsa/asoundlib.h> // Ensure you have ALSA development libraries
#include <cstring> // For std::to_string

AudioManager::AudioManager() : capturing(false) {}

AudioManager::~AudioManager() {
    stopCapture();
}

bool AudioManager::initialize() {
    selectedCaptureDevice = autoSelectFirstAlsaCaptureDevice();
    if (selectedCaptureDevice.empty()) {
        std::cerr << "Failed to select an ALSA capture device." << std::endl;
        return false;
    }

    mic = std::make_shared<PulseAudioMic>(audioQueue);
    if (!mic) {
        std::cerr << "Failed to create PulseAudioMic!" << std::endl;
        return false;
    }

    return true;
}

void AudioManager::startCapture() {
    if (mic && !capturing.load()) {
        capturing = true;
        micThread = std::thread(&PulseAudioMic::micRun, mic);
    }
}

void AudioManager::stopCapture() {
    if (mic && capturing.load()) {
        capturing = false;
        mic->micStop();
        if (micThread.joinable()) {
            micThread.join();
        }
    }
}

ThreadSafeQueue<std::vector<std::tuple<double, double>>>& AudioManager::getAudioQueue() {
    return audioQueue;
}

ThreadSafeQueue<std::vector<std::tuple<double, double>>>& AudioManager::getEmptyAudioQueue() {
    return emptyAudioQueue;
}

std::vector<std::string> AudioManager::listAlsaCaptureDevices() {
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

std::string AudioManager::autoSelectFirstAlsaCaptureDevice() {
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
    ThreadSafeQueue<std::vector<std::tuple<double, double>>> emptyAudioQueue;

    std::shared_ptr<PulseAudioMic> mic = std::make_shared<PulseAudioMic>(audioQueue);
    if (!mic) {
        std::cerr << "Failed to create PulseAudioMic!" << std::endl;
        return "";
    }
    std::thread micThread(&PulseAudioMic::micRun, mic);

    if (devices.empty()) {
        std::cerr << "Error: No ALSA capture devices found." << std::endl;
        return "";
    }
    std::cout << "Automatically selected ALSA capture device: " << devices[0] << std::endl;
    return devices[0];
}
