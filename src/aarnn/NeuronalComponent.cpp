#include "NeuronalComponent.h"

NeuronalComponent::NeuronalComponent(std::shared_ptr<Position> position)
        : position(position),
          energyLevel(100.0),
          maxEnergyLevel(100.0),
          energyConsumptionRate(5.0),
          energyReplenishRate(2.0)
{
}

void NeuronalComponent::updatePosition(const std::shared_ptr<Position>& newPosition)
{
    position = newPosition;
}

std::shared_ptr<Position> NeuronalComponent::getPosition() const
{
    return position;
}

void NeuronalComponent::initialise()
{
    if (!instanceInitialised)
    {
        // Initialization logic if needed
        // instanceInitialised = true;
    }
}

double NeuronalComponent::getEnergyLevel() const
{
    return energyLevel;
}

void NeuronalComponent::energyTopup(double amount)
{
    energyLevel += amount;
    if (energyLevel > maxEnergyLevel)
    {
        energyLevel = maxEnergyLevel;
    }
}

void NeuronalComponent::energyDrain(double amount)
{
    energyLevel -= amount;
    if (energyLevel < 0.0)
    {
        energyLevel = 0.0;
    }
}

void NeuronalComponent::useEnergy(double amount)
{
    energyDrain(amount);
}

void NeuronalComponent::updateEnergy(double deltaTime)
{
    // Simulate energy replenishment over time
    energyTopup(energyReplenishRate * deltaTime);
    // Simulate energy consumption for maintenance
    energyDrain(energyConsumptionRate * deltaTime);
}
