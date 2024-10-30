#include "NeuronalComponent.h"

NeuronalComponent::NeuronalComponent(std::shared_ptr<Position> position, std::weak_ptr<NeuronalComponent> parent)
        : position(position), parent(parent),
          energyLevel(100.0),
          maxEnergyLevel(100.0),
          energyConsumptionRate(0.005),
          energyReplenishRate(0.002)
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

void NeuronalComponent::setParent(std::weak_ptr<NeuronalComponent> parentComponent)
{
    parent = parentComponent;

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
    // Simulate energy consumption for maintenance
    energyDrain(energyConsumptionRate * deltaTime);

    // Simulate energy replenishment from parent
    double replenishAmount = energyReplenishRate * deltaTime;

    // Lock the weak_ptr to get a shared_ptr
    if (auto parentShared = parent.lock())
    {
        // Draw energy from parent if available
        double availableEnergy = std::min(replenishAmount, parentShared->getEnergyLevel());
        energyTopup(availableEnergy);
        parentShared->energyDrain(availableEnergy);
    }
    else
    {
        // For root components without a parent, replenish from external source
        if ( energyLevel == 0.0 ) {
            //energyTopup(replenishAmount);
            energyTopup(100.0);
            std::cout << "Energy replenished" << std::endl;
        }
    }
}

