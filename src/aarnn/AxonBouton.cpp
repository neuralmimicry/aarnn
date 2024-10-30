#include "AxonBouton.h"
#include "SynapticGap.h"
#include "Axon.h"
#include "Neuron.h"
#include <iostream>

AxonBouton::AxonBouton(const std::shared_ptr<Position>& position, std::weak_ptr<NeuronalComponent> parent)
        : NeuronalComponent(position, parent)
{
    // Additional initialization if needed
}

void AxonBouton::initialise()
{
    NeuronalComponent::initialise(); // Call base class initialise

    if (!instanceInitialised)
    {
        std::cout << "Initialising AxonBouton" << std::endl;

        if (!onwardSynapticGap)
        {
            onwardSynapticGap = std::make_shared<SynapticGap>(
                    std::make_shared<Position>(position->x + 1, position->y + 1, position->z + 1), std::static_pointer_cast<NeuronalComponent>(shared_from_this()));
        }
        onwardSynapticGap->initialise();
        onwardSynapticGap->updateFromAxonBouton(std::static_pointer_cast<AxonBouton>(shared_from_this()));

        instanceInitialised = true;
    }
}

void AxonBouton::update(double deltaTime)
{
    // Update energy levels
    updateEnergy(deltaTime);

    // Additional updates if necessary
    if (onwardSynapticGap)
    {
        onwardSynapticGap->update(deltaTime);
    }
}

void AxonBouton::addSynapticGap(const std::shared_ptr<SynapticGap>& gap)
{
    gap->updateFromAxonBouton(std::static_pointer_cast<AxonBouton>(shared_from_this()));
}

void AxonBouton::connectSynapticGap(std::shared_ptr<SynapticGap> gap)
{
    // Implement the connection logic
    onwardSynapticGap = std::move(gap);
    onwardSynapticGap->updateFromAxonBouton(std::static_pointer_cast<AxonBouton>(shared_from_this()));
}

std::shared_ptr<SynapticGap> AxonBouton::getSynapticGap() const
{
    return onwardSynapticGap;
}

void AxonBouton::setNeuron(std::weak_ptr<Neuron> parentNeuron)
{
    neuron = std::move(parentNeuron);
}

void AxonBouton::updateFromAxon(std::shared_ptr<Axon> parentAxonPointer)
{
    parentAxon = std::move(parentAxonPointer);
}

std::shared_ptr<Axon> AxonBouton::getParentAxon() const
{
    return parentAxon;
}

void AxonBouton::setAxonBoutonId(int id)
{
    axonBoutonId = id;
}

int AxonBouton::getAxonBoutonId() const
{
    return axonBoutonId;
}
