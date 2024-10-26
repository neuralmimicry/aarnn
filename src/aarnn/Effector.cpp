#include "Effector.h"
#include "SynapticGap.h"
#include <utility>

Effector::Effector(const std::shared_ptr<Position>& position)
        : NeuronalComponent(position)
{
    // Additional initialization if needed
}

void Effector::initialise()
{
    NeuronalComponent::initialise(); // Call base class initialise

    if (!instanceInitialised)
    {
        // Additional initialization logic if needed
        instanceInitialised = true;
    }
}

void Effector::addSynapticGap(std::shared_ptr<SynapticGap> synapticGap)
{
    synapticGaps.emplace_back(std::move(synapticGap));
}

std::vector<std::shared_ptr<SynapticGap>> Effector::getSynapticGaps() const
{
    return synapticGaps;
}

