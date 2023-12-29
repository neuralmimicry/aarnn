#ifndef AARNN_H
#define AARNN_H

#include "aarnn.h"
#include "avTimerCallback.h"
#include "Axon.h"
#include "AxonBouton.h"
#include "AxonBranch.h"
#include "AxonHillock.h"
#include "BodyComponent.h"
#include "BodyShapeComponent.h"
#include "Dendrite.h"
#include "DendriteBranch.h"
#include "DendriteBouton.h"
#include "Effector.h"
#include "Logger.h"
#include "Neuron.h"
#include "NeuronParameters.h"
#include "nvTimerCallback.h"
#include "portaudioMicThread.h"
#include "Position.h"
#include "PulseAudioMic.h"
#include "SensoryReceptor.h"
#include "Shape3D.h"
#include "Soma.h"
#include "SynapticGap.h"
#include "ThreadSafeQueue.h"

// Forward declarations
class avTimerCallback;
class Logger;
class BodyParameters;
class NeuronParameters;
class Position;
class Shape3D;
class BodyShapeComponent;
class NeuronShapeComponent;
class SensoryReceptor;
class Effector;
class SynapticGap;
class AxonBouton;
class DendriteBouton;
class Soma;
class AxonBranch;
class Axon;
class AxonHillock;
class Soma;
class Neuron;
class ThreadSafeQueue;

//void computePropagationRate(Neuron* neuron);
//void initialize_database(pqxx::connection& conn);
#endif
