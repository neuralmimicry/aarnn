//
// Created by pbisaacs on 24/06/24.
//

#ifndef AARNN_GLOBALS_H
#define AARNN_GLOBALS_H

// globals.h

#include <atomic>
#include <unordered_set>
#include <mutex>
#include <condition_variable>
#include "Neuron.h"

extern std::unordered_set<std::shared_ptr<Neuron>> changedNeurons;
extern std::mutex changedNeuronsMutex;
extern std::atomic<bool> running;
extern std::condition_variable cv;
extern std::atomic<bool> dbUpdateReady;

#endif //AARNN_GLOBALS_H
