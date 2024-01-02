#ifndef PORTAUDIOMICTHREAD_H
#define PORTAUDIOMICTHREAD_H

#include <iostream>
#include <iomanip>
#include <mutex>
#include <queue>
#include <fftw3.h>
#include <portaudio.h>
#include <pulse/pulseaudio.h>
#include <pulse/simple.h>
#include <pulse/error.h>
#include <pulse/proplist.h>

class portaudioMicThread
{
public:
    portaudioMicThread() : stream(nullptr) {}
    ~portaudioMicThread()
    {
        if (stream)
        {
            Pa_StopStream(stream);
            Pa_CloseStream(stream);
        }
        Pa_Terminate();
    }
    int run();
    void stop();

private:
    PaStream *stream;
    PaStreamInfo stream_info{};
};

#endif