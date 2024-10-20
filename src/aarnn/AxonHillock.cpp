#include "AxonHillock.h"

void AxonHillock::initialise()
{
    if (!instanceInitialised)
    {
        std::cout << "Initialising AxonHillock" << std::endl;

        if (!this->onwardAxon)
        {
            std::cout << "Creating Axon" << std::endl;
            this->onwardAxon = std::make_shared<Axon>(
                    std::make_shared<Position>((position->x) + 1, (position->y) + 1, (position->z) + 1));
        }

        std::cout << "AxonHillock initialising Axon" << std::endl;
        this->onwardAxon->initialise();
        std::cout << "AxonHillock updating from Axon" << std::endl;
        this->onwardAxon->updateFromAxonHillock(shared_from_this());

        instanceInitialised = true;
    }
    std::cout << "AxonHillock initialised" << std::endl;
}

void AxonHillock::updatePosition(const PositionPtr& newPosition)
{
    position = newPosition;
}

void AxonHillock::updateFromSoma(std::shared_ptr<Soma> parentPointer)
{
    this->parentSoma = std::move(parentPointer);
}
