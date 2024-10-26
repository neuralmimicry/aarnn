#ifndef NEURONALCOMPONENT_H
#define NEURONALCOMPONENT_H

#include <memory>
#include "Position.h"

class NeuronalComponent : public std::enable_shared_from_this<NeuronalComponent>
{
protected:
    std::shared_ptr<Position> position;
    bool instanceInitialised = false;

    // Energy management attributes
    double energyLevel;
    double maxEnergyLevel;
    double energyConsumptionRate;
    double energyReplenishRate;

public:
    explicit NeuronalComponent(std::shared_ptr<Position> position);

    // Position management
    virtual void updatePosition(const std::shared_ptr<Position>& newPosition);
    std::shared_ptr<Position> getPosition() const;

    // Initialization
    virtual void initialise();

    // Energy management
    virtual double getEnergyLevel() const;
    virtual void energyTopup(double amount);
    virtual void energyDrain(double amount);
    virtual void useEnergy(double amount);
    virtual void updateEnergy(double deltaTime);

    // Destructor
    virtual ~NeuronalComponent() = default;
};

#endif // NEURONALCOMPONENT_H
