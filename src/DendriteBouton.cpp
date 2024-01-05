#include "DendriteBouton.h"

void DendriteBouton::initialise()
{
    if (!instanceInitialised)
    {
        instanceInitialised = true;
    }
}

void DendriteBouton::connectSynapticGap(std::shared_ptr<SynapticGap> gap){
    onwardSynapticGap = std::move(gap);
    if (auto spt = neuron.lock()) { // has to check if the shared_ptr is still valid
        spt->addSynapticGapDendrite(onwardSynapticGap);
    }
}

void DendriteBouton::updatePosition(const PositionPtr &newPosition)
{
    position = newPosition;
}

void DendriteBouton::setNeuron(std::weak_ptr<Neuron> parentNeuron)
{
    this->neuron = std::move(parentNeuron);
}

