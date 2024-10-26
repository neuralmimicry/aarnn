#include "AxonHillock.h"
#include "Axon.h"
#include "Soma.h"
#include <iostream>

AxonHillock::AxonHillock(const std::shared_ptr<Position>& position)
        : NeuronalComponent(position)
{
    // Additional initialization if needed
}

void AxonHillock::initialise()
{
    NeuronalComponent::initialise(); // Call base class initialise

    if (!instanceInitialised)
    {
        std::cout << "Initialising AxonHillock" << std::endl;

        if (!onwardAxon)
        {
            std::cout << "Creating Axon" << std::endl;
            onwardAxon = std::make_shared<Axon>(
                    std::make_shared<Position>(position->x + 1, position->y + 1, position->z + 1));
        }

        std::cout << "AxonHillock initialising Axon" << std::endl;
        onwardAxon->initialise();
        std::cout << "AxonHillock updating from Axon" << std::endl;
        onwardAxon->updateFromAxonHillock(std::static_pointer_cast<AxonHillock>(shared_from_this()));

        instanceInitialised = true;
    }
    std::cout << "AxonHillock initialised" << std::endl;
}

void AxonHillock::update(double deltaTime)
{
    // Update energy levels
    updateEnergy(deltaTime);

    // Update the onward axon
    if (onwardAxon)
    {
        onwardAxon->update(deltaTime);
    }

    // Additional updates if necessary
}

std::shared_ptr<Axon> AxonHillock::getAxon() const
{
    return onwardAxon;
}

void AxonHillock::updateFromSoma(std::shared_ptr<Soma> parentPointer)
{
    parentSoma = std::move(parentPointer);
}

std::shared_ptr<Soma> AxonHillock::getParentSoma() const
{
    return parentSoma;
}

