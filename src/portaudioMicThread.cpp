#include "../include/portaudioMicThread.h"
#include "../include/utils.h"

static int portaudioMicCallBack(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void *userData)
{
    std::cout << "MicCallBack" << std::endl;
    // Read data from the stream.
    const SAMPLE *in = (const SAMPLE *) inputBuffer;
    SAMPLE *out = (SAMPLE *) outputBuffer;
    (void) timeInfo; /* Prevent unused variable warnings. */
    (void) statusFlags;
    (void) userData;

    if (inputBuffer == nullptr) {
        for (unsigned int i = 0; i < framesPerBuffer; i++) {
            *out++ = 0;  /* left - silent */
            *out++ = 0;  /* right - silent */
        }
        gNumNoInputs += 1;
    } else {
        for (unsigned int i = 0; i < framesPerBuffer; i++) {
            // Here you might want to capture audio data
            //capturedAudio.push_back(in[i]);
        }
    }
    return paContinue;
}


int portaudioMicThread::run()
{
    // Open the audio device.
    PaError err;
    std::cout << "PortAudio version: " << Pa_GetVersionText() << std::endl;
    err = Pa_Initialize();
    if (err != paNoError)
    {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        return 1;
    }

    std::vector<PaStreamParameters> inputParameters;
    std::vector<PaStreamParameters> outputParameters;
    int inputDeviceSelected = -1;
    int outputDeviceSelected = -1;
    PaDeviceIndex indexLoop = 0;

    int numHostApis = Pa_GetHostApiCount();
    for (int i = 0; i < numHostApis; ++i)
    {
        const PaHostApiInfo *hostApiInfo = Pa_GetHostApiInfo(i);
        if (hostApiInfo)
        {
            std::cout << "Host API #" << i << std::endl;
            std::cout << "  Name:               " << hostApiInfo->name << std::endl;
            std::cout << "  Type:               " << hostApiInfo->type << std::endl;
            std::cout << "  Device Count:       " << hostApiInfo->deviceCount << std::endl;
            std::cout << "  Default Input Dev:  " << hostApiInfo->defaultInputDevice << std::endl;
            std::cout << "  Default Output Dev: " << hostApiInfo->defaultOutputDevice << std::endl;
            std::cout << std::endl;
        }
    }

    int numDevices = Pa_GetDeviceCount();

    if (numDevices < 0)
    {
        std::cerr << "Pa_CountDevices returned " << numDevices << std::endl;
        return 1;
    }

    PaSampleFormat sampleFormats[] = {
        paFloat32,
        paInt32,
        paInt24,
        paInt16,
        paInt8,
        paUInt8};

    double sampleRates[] = {
        8000.0,
        16000.0,
        22050.0,
        32000.0,
        44100.0,
        48000.0,
        96000.0,
        192000.0};

    std::cout << "\n\nInput Devices Info\n"
              << std::endl;
    std::cout << std::left << std::setw(5) << "Index"
              << std::setw(20) << "Device Name"
              << std::setw(20) << "Host API"
              << std::setw(20) << "Max Input Channels"
              << std::setw(30) << "Supported Sample Rates"
              << std::setw(30) << "Supported Sample Formats" << std::endl;

    std::cout << std::string(140, '-') << std::endl;

    for (int i = 0; i < numDevices; i++)
    {
        const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo(i);

        std::cout << std::left << std::setw(5) << i
                  << std::setw(20) << deviceInfo->name
                  << std::setw(20) << Pa_GetHostApiInfo(deviceInfo->hostApi)->name
                  << std::setw(20) << deviceInfo->maxInputChannels;

        std::string sampleRatesStr = "";
        std::string sampleFormatsStr = "";
        indexLoop = i;
        inputParameters.emplace_back();
        inputParameters.back().device = indexLoop;
        inputParameters.back().channelCount = deviceInfo->maxInputChannels;
        inputParameters.back().sampleFormat = paInt16; // default to paInt16
        inputParameters.back().suggestedLatency = deviceInfo->defaultLowInputLatency;
        inputParameters.back().hostApiSpecificStreamInfo = NULL;

        for (double sampleRate : sampleRates)
        {
            err = Pa_IsFormatSupported(&inputParameters.back(), NULL, sampleRate);
            if (err == paFormatIsSupported)
            {
                sampleRatesStr += std::to_string((int)sampleRate) + ", ";
            }
        }

        for (PaSampleFormat sampleFormat : sampleFormats)
        {
            inputParameters.back().sampleFormat = sampleFormat;
            err = Pa_IsFormatSupported(&inputParameters.back(), NULL, 44100.0); // default to 44100.0 Hz
            if (err == paFormatIsSupported)
            {
                sampleFormatsStr += std::to_string(sampleFormat) + ", ";
            }
        }

        std::cout << std::setw(30) << sampleRatesStr << std::setw(30) << sampleFormatsStr << std::endl;
    }

    // User selects the input device
    std::cout << "\nSelect the index of the input device you want to use: ";
    std::cin >> inputDeviceSelected;
    if (inputDeviceSelected < 0 || inputDeviceSelected >= numDevices)
    {
        std::cerr << "Invalid input device index" << std::endl;
        return 1;
    }

    // User selects channel count for input
    int selectedInputChannels;
    std::cout << "Enter the number of input channels (e.g., 1 for mono, 2 for stereo): ";
    std::cin >> selectedInputChannels;

    std::cout << std::left << std::setw(5) << "Index"
              << std::setw(20) << "Device Name"
              << std::setw(20) << "Host API"
              << std::setw(20) << "Max Output Channels"
              << std::setw(30) << "Supported Sample Rates"
              << std::setw(30) << "Supported Sample Formats"
              << std::endl;

    std::cout << std::string(140, '-') << std::endl;

    for (int i = 0; i < numDevices; i++)
    {
        const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo(i);

        std::cout << std::left << std::setw(5) << i
                  << std::setw(20) << deviceInfo->name
                  << std::setw(20) << Pa_GetHostApiInfo(deviceInfo->hostApi)->name
                  << std::setw(20) << deviceInfo->maxOutputChannels;

        std::string sampleRatesStr = "";
        std::string sampleFormatsStr = "";

        indexLoop = i;
        outputParameters.emplace_back();
        outputParameters.back().device = indexLoop;
        outputParameters.back().channelCount = deviceInfo->maxOutputChannels;
        outputParameters.back().sampleFormat = paInt16; // default to paInt16
        outputParameters.back().suggestedLatency = deviceInfo->defaultLowOutputLatency;
        outputParameters.back().hostApiSpecificStreamInfo = NULL;

        for (double sampleRate : sampleRates)
        {
            err = Pa_IsFormatSupported(NULL, &outputParameters.back(), sampleRate);
            if (err == paFormatIsSupported)
            {
                sampleRatesStr += std::to_string((int)sampleRate) + ", ";
            }
        }

        for (PaSampleFormat sampleFormat : sampleFormats)
        {
            outputParameters.back().sampleFormat = sampleFormat;
            err = Pa_IsFormatSupported(NULL, &outputParameters.back(), 44100.0); // default to 44100.0 Hz
            if (err == paFormatIsSupported)
            {
                sampleFormatsStr += std::to_string(sampleFormat) + ", ";
            }
        }

        std::cout << std::setw(30) << sampleRatesStr
                  << std::setw(30) << sampleFormatsStr << std::endl;
    }

    // User selects the output device
    std::cout << "Select the index of the output device you want to use: ";
    std::cin >> outputDeviceSelected;
    if (outputDeviceSelected < 0 || outputDeviceSelected >= numDevices)
    {
        std::cerr << "Invalid output device index" << std::endl;
        return 1;
    }

    // User selects channel count for output
    int selectedOutputChannels;
    std::cout << "Enter the number of output channels (e.g., 1 for mono, 2 for stereo): ";
    std::cin >> selectedOutputChannels;

    // Setup input parameters
    inputParameters[inputDeviceSelected].device = inputDeviceSelected;
    inputParameters[inputDeviceSelected].channelCount = selectedInputChannels;
    inputParameters[inputDeviceSelected].sampleFormat = paInt16; // default to paInt16
    inputParameters[inputDeviceSelected].suggestedLatency = Pa_GetDeviceInfo(inputDeviceSelected)->defaultLowInputLatency;
    inputParameters[inputDeviceSelected].hostApiSpecificStreamInfo = NULL;

    // Setup output parameters
    outputParameters[outputDeviceSelected].device = outputDeviceSelected;
    outputParameters[outputDeviceSelected].channelCount = selectedOutputChannels;
    outputParameters[outputDeviceSelected].sampleFormat = paInt16; // default to paInt16
    outputParameters[outputDeviceSelected].suggestedLatency = Pa_GetDeviceInfo(outputDeviceSelected)->defaultLowOutputLatency;
    outputParameters[outputDeviceSelected].hostApiSpecificStreamInfo = NULL;

    // Open the stream.
    err = Pa_OpenStream(&stream, &inputParameters[inputDeviceSelected], &outputParameters[outputDeviceSelected], 
                        SAMPLE_RATE, FRAMES_PER_BUFFER, paClipOff, portaudioMicCallBack, NULL);
    if (err != paNoError)
    {
        std::cout << "Error opening audio device: " << err << std::endl;
        return 1;
    }

    // Start the stream.
    err = Pa_StartStream(stream);
    if (err != paNoError)
    {
        std::cout << "Error starting audio stream: " << err << std::endl;
        return 1;
    }
    stream_info = *Pa_GetStreamInfo(stream);
    return 0;
}

void portaudioMicThread::stop()
{
    // Close the stream.
    Pa_CloseStream(stream);
}
