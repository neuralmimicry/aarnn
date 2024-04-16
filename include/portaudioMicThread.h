#ifndef PORTAUDIOMICTHREAD_H_INCLUDED
#define PORTAUDIOMICTHREAD_H_INCLUDED

#include <fftw3.h>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <portaudio.h>
#include <pulse/error.h>
#include <pulse/proplist.h>
#include <pulse/pulseaudio.h>
#include <pulse/simple.h>
#include <queue>

class portaudioMicThread
{
    public:
    portaudioMicThread() : stream(nullptr)
    {
    }
    ~portaudioMicThread()
    {
        if(stream)
        {
            Pa_StopStream(stream);
            Pa_CloseStream(stream);
        }
        Pa_Terminate();
    }
    int  run();
    void stop();

    private:
    PaStream    *stream;
    PaStreamInfo stream_info{};
};

#endif  // PORTAUDIOMICTHREAD_H_INCLUDED
