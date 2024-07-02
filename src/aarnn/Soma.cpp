#include "Soma.h"

#include "utils.h"

void Soma::initialise()
{
    if(!instanceInitialised)
    {
        std::cout << "Initialising Soma" << std::endl;
        if(!onwardAxonHillock)
        {
            std::cout << "Creating AxonHillock" << std::endl;
            onwardAxonHillock = std::make_shared<AxonHillock>(
             std::make_shared<Position>((position->x) + 1, (position->y) + 1, (position->z) + 1));
        }
        std::cout << "Soma initialising AxonHillock" << std::endl;
        onwardAxonHillock->initialise();
        std::cout << "Soma updating from AxonHillock" << std::endl;
        onwardAxonHillock->updateFromSoma(shared_from_this());
        std::cout << "Creating DendriteBranch" << std::endl;
        addDendriteBranch(std::make_shared<DendriteBranch>(
         std::make_shared<Position>((position->x) - 1, (position->y) - 1, (position->z) - 1)));
        std::cout << "Soma initialising DendriteBranch" << std::endl;
        dendriteBranches.back()->initialise();
        std::cout << "Soma updating from DendriteBranch" << std::endl;
        dendriteBranches.back()->updateFromSoma(shared_from_this());
        instanceInitialised = true;
    }
    std::cout << "Soma initialised" << std::endl;
}

void Soma::updatePosition(const PositionPtr &newPosition)
{
    position = newPosition;
}

void Soma::addDendriteBranch(std::shared_ptr<DendriteBranch> dendriteBranch)
{
    auto        coords = get_coordinates(int(dendriteBranches.size() + 1), int(dendriteBranches.size() + 1), int(5));
    PositionPtr currentPosition = dendriteBranch->getPosition();
    auto        x               = double(std::get<0>(coords)) + currentPosition->x;
    auto        y               = double(std::get<1>(coords)) + currentPosition->y;
    auto        z               = double(std::get<2>(coords)) + currentPosition->z;
    currentPosition->x          = x;
    currentPosition->y          = y;
    currentPosition->z          = z;
    dendriteBranches.emplace_back(std::move(dendriteBranch));
}

void Soma::updateFromNeuron(std::shared_ptr<Neuron> parentPointer)
{
    parentNeuron = std::move(parentPointer);
}