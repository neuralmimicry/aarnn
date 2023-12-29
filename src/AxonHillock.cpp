#include <iostream>

#include "../include/AxonHillock.h"

explicit AxonHillock::AxonHillock(const PositionPtr &position) : BodyComponent(position), BodyShapeComponent()
{
    // On construction set a default propagation rate
    propagationRate = 0.5;
}
~AxonHillock::AxonHillock() override = default;

void AxonHillock::initialise()
{
    if (!instanceInitialised)
    {
        if (!onwardAxon)
        {
            onwardAxon = std::make_shared<Axon>(std::make_shared<Position>((position->x) + 1, (position->y) + 1, (position->z) + 1));
        }
        onwardAxon->initialise();
        onwardAxon->updateFromAxonHillock(shared_from_this());
        instanceInitialised = true;
    }
}

void AxonHillock::updatePosition(const PositionPtr &newPosition)
{
    position = newPosition;
}

void updateFromSoma(std::shared_ptr<Soma> parentPointer) { parentSoma = std::move(parentPointer); }
