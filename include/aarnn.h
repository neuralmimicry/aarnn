#ifndef AARNN_H
#define AARNN_H
// Forward declarations
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

void computePropagationRate(Neuron* neuron);
void initialize_database(pqxx::connection& conn);
#endif
