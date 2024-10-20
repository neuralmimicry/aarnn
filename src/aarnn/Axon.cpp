#include "Axon.h"

void Axon::initialise()
{
    if(!instanceInitialised)
    {
        std::cout << "Initialising Axon" << std::endl;
        if(!this->onwardAxonBouton)
        {
            this->onwardAxonBouton = std::make_shared<AxonBouton>(
             std::make_shared<Position>((position->x) + 1, (position->y) + 1, (position->z) + 1));
        }
        this->onwardAxonBouton->initialise();
        this->onwardAxonBouton->updateFromAxon(shared_from_this());
        instanceInitialised = true;
    }
}

void Axon::updatePosition(const std::shared_ptr<Position> &newPosition)
{
    position = newPosition;
}

void Axon::addBranch(std::shared_ptr<AxonBranch> branch)
{
    axonBranches.push_back(branch);
}

const std::vector<std::shared_ptr<AxonBranch>> &Axon::getAxonBranches() const
{
    return axonBranches;
}

std::shared_ptr<AxonBouton> Axon::getAxonBouton() const
{
    return onwardAxonBouton;
}

double Axon::calcPropagationTime()
{
    // Implement the calculation
    return 0.0;
}

void Axon::updateFromAxonHillock(std::shared_ptr<AxonHillock> parentAxonHillockPointer)
{
    this->parentAxonHillock = std::move(parentAxonHillockPointer);
}

std::shared_ptr<AxonHillock> Axon::getParentAxonHillock() const
{
    return this->parentAxonHillock;
}

void Axon::updateFromAxonBranch(std::shared_ptr<AxonBranch> parentAxonBranchPointer)
{
    this->parentAxonBranch = std::move(parentAxonBranchPointer);
}

std::shared_ptr<AxonBranch> Axon::getParentAxonBranch() const
{
    return this->parentAxonBranch;
}
