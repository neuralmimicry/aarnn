#include "AxonBranch.h"

#include "utils.h"

void AxonBranch::initialise()
{
    if(!instanceInitialised)
    {
        if(onwardAxons.empty())
        {
            connectAxon(std::make_shared<Axon>(
             std::make_shared<Position>((position->x) + 1, (position->y) + 1, (position->z) + 1)));
            onwardAxons.back()->initialise();
            onwardAxons.back()->updateFromAxonBranch(shared_from_this());
        }
        instanceInitialised = true;
    }
}

void AxonBranch::updatePosition(const PositionPtr &newPosition)
{
    position = newPosition;
}

void AxonBranch::connectAxon(std::shared_ptr<Axon> axon)
{
    auto        coords          = get_coordinates(int(onwardAxons.size() + 1), int(onwardAxons.size() + 1), int(5));
    PositionPtr currentPosition = axon->getPosition();
    auto        x               = double(std::get<0>(coords)) + currentPosition->x;
    auto        y               = double(std::get<1>(coords)) + currentPosition->y;
    auto        z               = double(std::get<2>(coords)) + currentPosition->z;
    currentPosition->x          = x;
    currentPosition->y          = y;
    currentPosition->z          = z;
    onwardAxons.emplace_back(std::move(axon));
}

void AxonBranch::updateFromAxon(std::shared_ptr<Axon> parentPointer)
{
    parentAxon = std::move(parentPointer);
}
