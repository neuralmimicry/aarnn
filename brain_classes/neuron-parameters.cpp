#include "../include/neuron-parameters.h"

NeuronParameters::NeuronParameters()
{
    // Access the database and load neuron parameters
}

double NeuronParameters::getDendritePropagationRate() const
{
    return dendritePropagationRate;
}
