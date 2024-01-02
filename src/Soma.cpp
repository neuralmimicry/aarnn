#include "../include/Soma.h"
#include "../include/utils.h"

void Soma::initialise()
{
    if (!instanceInitialised)
    {
        if (!onwardAxonHillock)
        {
            onwardAxonHillock = std::make_shared<AxonHillock>(std::make_shared<Position>((position->x) + 1, (position->y) + 1, (position->z) + 1));
        }
        onwardAxonHillock->initialise();
        onwardAxonHillock->updateFromSoma(shared_from_this());
        addDendriteBranch(std::make_shared<DendriteBranch>(std::make_shared<Position>((position->x) - 1, (position->y) - 1, (position->z) - 1)));
        dendriteBranches.back()->initialise();
        dendriteBranches.back()->updateFromSoma(shared_from_this());
        instanceInitialised = true;
    }
}

void Soma::updatePosition(const PositionPtr &newPosition)
{
    position = newPosition;
}

void Soma::addDendriteBranch(std::shared_ptr<DendriteBranch> dendriteBranch)
{
    auto coords = get_coordinates(int(dendriteBranches.size() + 1), int(dendriteBranches.size() + 1), int(5));
    PositionPtr currentPosition = dendriteBranch->getPosition();
    auto x = double(std::get<0>(coords)) + currentPosition->x;
    auto y = double(std::get<1>(coords)) + currentPosition->y;
    auto z = double(std::get<2>(coords)) + currentPosition->z;
    currentPosition->x = x;
    currentPosition->y = y;
    currentPosition->z = z;
    dendriteBranches.emplace_back(std::move(dendriteBranch));
}

void Soma::updateFromNeuron(std::shared_ptr<Neuron> parentPointer) { parentNeuron = std::move(parentPointer); }