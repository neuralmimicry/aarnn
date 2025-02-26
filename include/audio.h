//
// Created by pbisaacs on 23/06/24.
//

#ifndef AARNN_AUDIO_H
#define AARNN_AUDIO_H

#include <portaudio.h>
#include "PulseAudioMic.h"
#include "utils.h"

static int portaudioMicCallBack(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void *userData);
void runInteractor(std::vector<std::shared_ptr<Neuron>>& neurons, std::mutex& neuron_mutex, ThreadSafeQueue<std::vector<std::tuple<double, double>>>& audioQueue, int whichCallBack);

#endif //AARNN_AUDIO_H
