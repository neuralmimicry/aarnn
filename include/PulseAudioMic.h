#ifndef PULSEAUDIOMIC_H
#define PULSEAUDIOMIC_H

#include "ThreadSafeQueue.h"
#include <cmath>
#include <fftw3.h>
#include <iostream>
#include <mutex>
#include <portaudio.h>
#include <pulse/error.h>
#include <pulse/proplist.h>
#include <pulse/pulseaudio.h>
#include <pulse/simple.h>
#include <queue>

class PulseAudioMic
{
    public:
    explicit PulseAudioMic(ThreadSafeQueue<std::vector<std::tuple<double, double>>> &audioQueue);
    ~PulseAudioMic()
    {
        cleanUp();
    }
    void                                    sourceSelection();
    int                                     micRun();
    std::vector<std::string>                getSources();
    std::vector<std::tuple<double, double>> getCapturedAuditory();
    void                                    setSource(const std::string &source);
    void                                    readStream();
    void                                    micStop();

    private:
    pa_threaded_mainloop                                     *mainloop     = nullptr;
    pa_mainloop_api                                          *mainloop_api = nullptr;
    pa_context                                               *context      = nullptr;
    pa_stream                                                *stream       = nullptr;
    pa_sample_spec                                            sample_spec;
    std::vector<std::string>                                  sources;
    unsigned int                                              sample_rate{};
    std::vector<std::tuple<double, double>>                   capturedAuditory;
    std::mutex                                                capturedAuditory_mtx;
    ThreadSafeQueue<std::vector<std::tuple<double, double>>> &audioQueue;
    void                                                      initialiseContext();
    void                                                      initialiseCallbacks();
    void                                                      initialiseStream(const char *source);
    void                                                      cleanUp();
    void        processStream(const void *inputBuffer, size_t framesPerBuffer);
    static void context_state_callback_static(pa_context *c, void *userdata)
    {
        auto *self = reinterpret_cast<PulseAudioMic *>(userdata);
        self->context_state_callback(c);
    }
    void        context_state_callback(pa_context *c);
    static void source_info_callback_static(pa_context *c, const pa_source_info *i, int eol, void *userdata)
    {
        auto *self = reinterpret_cast<PulseAudioMic *>(userdata);
        self->source_info_callback(c, i, eol);
    }
    void        source_info_callback(pa_context *c, const pa_source_info *i, int eol);
    static void stream_state_callback_static(pa_stream *s, void *userdata)
    {
        auto *self = reinterpret_cast<PulseAudioMic *>(userdata);
        std::cout << "Stream state changed\n";
        self->stream_state_callback(s);
    }
    static void        stream_state_callback(pa_stream *s);
    static void stream_read_callback_static(pa_stream *s, size_t length, void *userdata)
    {
        auto *self = reinterpret_cast<PulseAudioMic *>(userdata);
        // std::cout << "O";
        self->readStream();
    }
};

#endif