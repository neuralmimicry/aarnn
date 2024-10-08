#include "AxonHillock.h"

void AxonHillock::initialise()
{
    if(!instanceInitialised)
    {
        std::cout << "Initialising AxonHillock" << std::endl;
        if(!onwardAxon)
        {
            onwardAxon = std::make_shared<Axon>(
             std::make_shared<Position>((position->x) + 1, (position->y) + 1, (position->z) + 1));
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

void AxonHillock::updateFromSoma(std::shared_ptr<Soma> parentPointer)
{
    parentSoma = std::move(parentPointer);
}
