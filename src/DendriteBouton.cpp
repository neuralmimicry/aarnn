#include <iostream>
#include "../include/DendriteBouton.h"

explicit DendriteBouton::DendriteBouton(const PositionPtr &position) : BodyComponent(position), BodyShapeComponent()
{
    // On construction set a default propagation rate
    propagationRate = 0.5;
}
~DendriteBouton::DendriteBouton() override = default;

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
y
void DendriteBouton::setNeuron(std::weak_ptr<Neuron> parentNeuron)
{
    this->neuron = std::move(parentNeuron);
}

