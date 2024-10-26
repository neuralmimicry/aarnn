#include "Axon.h"
#include "AxonBouton.h"
#include "AxonBranch.h"
#include "AxonHillock.h"
#include <iostream>

Axon::Axon(const std::shared_ptr<Position>& position)
        : NeuronalComponent(position)
{
    // Additional initialization if needed
}

void Axon::initialise()
{
    NeuronalComponent::initialise(); // Call base class initialise

    if (!instanceInitialised)
    {
        std::cout << "Initialising Axon" << std::endl;

        if (!onwardAxonBouton)
        {
            onwardAxonBouton = std::make_shared<AxonBouton>(
                    std::make_shared<Position>(position->x + 1, position->y + 1, position->z + 1));
        }
        onwardAxonBouton->initialise();
        onwardAxonBouton->updateFromAxon(std::static_pointer_cast<Axon>(shared_from_this()));

        instanceInitialised = true;
    }
}

void Axon::update(double deltaTime)
{
    // Update energy levels
    updateEnergy(deltaTime);

    // Propagate signal if possible
    //if (canPropagateSignal())
    //{
    //    propagateSignal();
    //}

    // Update onward Axon Bouton
    if (onwardAxonBouton)
    {
        onwardAxonBouton->update(deltaTime);
    }

    // Update branches
    for (auto& branch : axonBranches)
    {
        branch->update(deltaTime);
    }
}

void Axon::addBranch(std::shared_ptr<AxonBranch> branch)
{
    axonBranches.push_back(branch);
}

const std::vector<std::shared_ptr<AxonBranch>>& Axon::getAxonBranches() const
{
    return axonBranches;
}

std::shared_ptr<AxonBouton> Axon::getAxonBouton() const
{
    return onwardAxonBouton;
}

double Axon::calcPropagationTime()
{
    // Implement the calculation based on axon properties
    return 0.0;
}

void Axon::updateFromAxonHillock(std::shared_ptr<AxonHillock> parentAxonHillockPointer)
{
    parentAxonHillock = std::move(parentAxonHillockPointer);
}

std::shared_ptr<AxonHillock> Axon::getParentAxonHillock() const
{
    return parentAxonHillock;
}

void Axon::updateFromAxonBranch(std::shared_ptr<AxonBranch> parentAxonBranchPointer)
{
    parentAxonBranch = std::move(parentAxonBranchPointer);
}

std::shared_ptr<AxonBranch> Axon::getParentAxonBranch() const
{
    return parentAxonBranch;
}
