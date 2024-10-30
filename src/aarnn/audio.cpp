//
// Created by pbisaacs on 23/06/24.
//
#include "audio.h"
#include <iostream>
#include "Logger.h"
#include "utils.h"
#include "nvTimerCallback.h"
#include "timer.h"
#include "avTimerCallback.h"

static int portaudioMicCallBack(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void *userData) {
    std::cout << "MicCallBack" << std::endl;
    // Read data from the stream.
    const auto* in = (const SAMPLE*)inputBuffer;
    auto* out = (SAMPLE*)outputBuffer;
    (void)timeInfo; /* Prevent unused variable warnings. */
    (void)statusFlags;
    (void)userData;

    if (inputBuffer == nullptr) {
        for (unsigned int i = 0; i < framesPerBuffer; i++) {
            *out++ = 0;  /* left - silent */
            *out++ = 0;  /* right - silent */
        }
        gNumNoInputs += 1;
    } else {
        for (unsigned int i = 0; i < framesPerBuffer; i++) {
            // Here you might want to capture audio data
            //capturedAuditory.push_back(in[i]);
        }
    }
    return paContinue;
}

void runInteractor(std::vector<std::shared_ptr<Neuron>>& neurons, std::mutex& neuron_mutex, ThreadSafeQueue<std::vector<std::tuple<double, double>>>& audioQueue, int whichCallBack) {
    if (whichCallBack == 0) {
        vtkSmartPointer<nvTimerCallback> nvCB = vtkSmartPointer<nvTimerCallback>::New();
        nvCB->setNeurons(neurons);
        nvCB->setMutex(neuron_mutex);
        nvCB->nvRenderWindow->AddRenderer(nvCB->nvRenderer);

        while (true) {
            auto executeFunc = [nvCB]() mutable { nvCB->Execute(nullptr, vtkCommand::TimerEvent, nullptr); };
            logExecutionTime(executeFunc, "nvCB->Execute");
            nvCB->nvRenderWindow->Render();
            std::cout << "x";
        }
    }
    if (whichCallBack == 1) {
        vtkSmartPointer<avTimerCallback> avCB = vtkSmartPointer<avTimerCallback>::New();
        avCB->setAuditory(audioQueue);
        avCB->avRenderWindow->AddRenderer(avCB->avRenderer);

        while (true) {
            auto executeFunc = [avCB]() mutable { avCB->Execute(nullptr, vtkCommand::TimerEvent, nullptr); };
            logExecutionTime(executeFunc, "avCB->Execute");
            avCB->avRenderWindow->AddRenderer(avCB->avRenderer);
            avCB->avRenderWindow->Render();
            std::cout << "X";
        }
    }
}
