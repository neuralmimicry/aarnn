#include "SynapticGap.h"
#include "DendriteBouton.h"
#include "AxonBouton.h"
#include "SensoryReceptor.h"
#include "Effector.h"
#include <iostream>
#include <ctime>
#include <cmath>

SynapticGap::SynapticGap(const std::shared_ptr<Position>& position)
        : NeuronalComponent(position)
{
    // Additional initialization if needed
}

void SynapticGap::initialise()
{
    NeuronalComponent::initialise(); // Call base class initialise

    if (!instanceInitialised)
    {
        std::cout << "Initialising SynapticGap" << std::endl;
        // Initialization logic if needed
        instanceInitialised = true;
    }
}

void SynapticGap::update(double deltaTime)
{
    // Update energy levels
    updateEnergy(deltaTime);

    // Update component energy level
    // updateComponent(deltaTime, energyLevel);

    // Additional updates if necessary
    // Spike?
}

bool SynapticGap::isAssociated() const
{
    return associated;
}

void SynapticGap::setAsAssociated()
{
    associated = true;
}

void SynapticGap::updateFromSensoryReceptor(std::shared_ptr<SensoryReceptor> parentSensoryReceptorPointer)
{
    parentSensoryReceptor = std::move(parentSensoryReceptorPointer);
}

void SynapticGap::updateFromEffector(std::shared_ptr<Effector> parentEffectorPointer)
{
    parentEffector = std::move(parentEffectorPointer);
}

void SynapticGap::updateFromAxonBouton(std::shared_ptr<AxonBouton> parentAxonBoutonPointer)
{
    parentAxonBouton = std::move(parentAxonBoutonPointer);
}

void SynapticGap::updateFromDendriteBouton(std::shared_ptr<DendriteBouton> parentDendriteBoutonPointer)
{
    parentDendriteBouton = std::move(parentDendriteBoutonPointer);
}

void SynapticGap::updateComponent(double time, double energy)
{
    componentEnergyLevel = calculateEnergy(time, componentEnergyLevel + energy);  // Update the component energy level

    // Propagate energy or signal to connected components if necessary
}

double SynapticGap::calculateEnergy(double currentTime, double currentEnergyLevel)
{
    double deltaTime = currentTime - previousTime;
    previousTime = currentTime;
    energyLevel = currentEnergyLevel;

    double totalTime = attack + decay + sustain + release;
    double timeInCycle = fmod(deltaTime, totalTime);

    if (timeInCycle < attack)
    {
        // Attack phase
        return (timeInCycle / attack) * calculateWaveform(currentTime);
    }
    else if (timeInCycle < attack + decay)
    {
        // Decay phase
        double decayTime = timeInCycle - attack;
        return ((1 - decayTime / decay) * (1 - sustain) + sustain) * calculateWaveform(currentTime);
    }
    else if (timeInCycle < attack + decay + sustain)
    {
        // Sustain phase
        return sustain * calculateWaveform(currentTime);
    }
    else
    {
        // Release phase
        double releaseTime = timeInCycle - attack - decay - sustain;
        return (1 - std::max(0.0, releaseTime / release)) * calculateWaveform(currentTime);
    }
}

double SynapticGap::calculateWaveform(double currentTime) const
{
    return energyLevel * sin(2 * M_PI * frequencyResponse * currentTime + phaseShift);
}

double SynapticGap::propagationTime()
{
    double currentTime = static_cast<double>(std::clock()) / CLOCKS_PER_SEC;
    callCount++;
    double timeSinceLastCall = currentTime - lastCallTime;
    lastCallTime = currentTime;

    double x = 1 / (1 + exp(-callCount / (timeSinceLastCall + 1e-6))); // Added small value to prevent division by zero
    return minPropagationTime + x * (maxPropagationTime - minPropagationTime);
}

std::shared_ptr<SensoryReceptor> SynapticGap::getParentSensoryReceptor() const
{
    return parentSensoryReceptor;
}

std::shared_ptr<Effector> SynapticGap::getParentEffector() const
{
    return parentEffector;
}

std::shared_ptr<AxonBouton> SynapticGap::getParentAxonBouton() const
{
    return parentAxonBouton;
}

std::shared_ptr<DendriteBouton> SynapticGap::getParentDendriteBouton() const
{
    return parentDendriteBouton;
}

void SynapticGap::setSynapticGapId(int id)
{
    synapticGapId = id;
}

int SynapticGap::getSynapticGapId() const
{
    return synapticGapId;
}
