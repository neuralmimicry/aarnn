#include "DendriteBouton.h"
#include "SynapticGap.h"
#include "Neuron.h"
#include <utility>

DendriteBouton::DendriteBouton(const std::shared_ptr<Position>& position, std::weak_ptr<NeuronalComponent> parent)
        : NeuronalComponent(position, parent)
{
    // Additional initialization if needed
}

void DendriteBouton::initialise()
{
    NeuronalComponent::initialise(); // Call base class initialise

    if (!instanceInitialised)
    {
        // Additional initialization logic if needed
        instanceInitialised = true;
    }
}

void DendriteBouton::connectSynapticGap(std::shared_ptr<SynapticGap> gap)
{
    onwardSynapticGap = std::move(gap);
    if (auto neuronPtr = neuron.lock())
    {
        neuronPtr->addSynapticGapDendrite(onwardSynapticGap);
    }
}

void DendriteBouton::setNeuron(std::weak_ptr<Neuron> parentNeuron)
{
    neuron = std::move(parentNeuron);
}

void DendriteBouton::updateFromDendrite(std::shared_ptr<Dendrite> parentDendritePointer)
{
    parentDendrite = std::move(parentDendritePointer);
}

std::shared_ptr<Dendrite> DendriteBouton::getParentDendrite() const
{
    return parentDendrite;
}

std::shared_ptr<SynapticGap> DendriteBouton::getSynapticGap() const
{
    return onwardSynapticGap;
}

void DendriteBouton::update(double deltaTime)
{
    // Update energy levels
    updateEnergy(deltaTime);
}

void DendriteBouton::setDendriteBoutonId(int id)
{
    dendriteBoutonId = id;
}

int DendriteBouton::getDendriteBoutonId() const
{
    return dendriteBoutonId;
}

