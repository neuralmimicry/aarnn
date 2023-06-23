#include <iostream>
#include <portaudio.h>

int main() {
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        return 1;
    }

    int numDevices = Pa_GetDeviceCount();
    if (numDevices < 0) {
        std::cerr << "Pa_CountDevices returned 0x" << std::hex << numDevices << std::endl;
        err = numDevices;
        goto error;
    }

    const PaDeviceInfo* deviceInfo;
    for (int i = 0; i < numDevices; i++) {
        deviceInfo = Pa_GetDeviceInfo(i);
        std::cout << "--------------------------------------- device #" << i << std::endl;
        std::cout << "Name                     = " << deviceInfo->name << std::endl;
        std::cout << "Host API                 = " << Pa_GetHostApiInfo(deviceInfo->hostApi)->name << std::endl;
        std::cout << "Max inputs               = " << deviceInfo->maxInputChannels << std::endl;
        std::cout << "Max outputs              = " << deviceInfo->maxOutputChannels << std::endl;
    }

    // Set desired device here.
    PaStreamParameters outputParameters;
    outputParameters.device = 0;
    outputParameters.channelCount = 2;       // stereo output
    outputParameters.sampleFormat = paFloat32;
    outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = nullptr;

    // Here you can open a stream using the device and parameters you set.
    // ...

error:
    Pa_Terminate();
    if (err != paNoError) {
        std::cerr << "An error occurred while using the portaudio stream" << std::endl;
        std::cerr << "Error number: " << err << std::endl;
        std::cerr << "Error message: " << Pa_GetErrorText(err) << std::endl;
        return 1;
    }

    return 0;
}

