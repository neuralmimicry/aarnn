#include <iostream>
#include "../include/DendriteBouton.h"



void DendriteBouton::initialise()
{
    if (!instanceInitialised)
    {
        instanceInitialised = true;
    }
}

void connectSynapticGap(std::shared_ptr<SynapticGap> gap);

void DendriteBouton::updatePosition(const PositionPtr &newPosition)
{
    position = newPosition;
}

void DendriteBouton::setNeuron(std::weak_ptr<Neuron> parentNeuron)
{
    this->neuron = std::move(parentNeuron);
}

