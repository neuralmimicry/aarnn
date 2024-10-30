#include "DendriteBranch.h"
#include "Soma.h"
#include "utils.h"
#include <iostream>

DendriteBranch::DendriteBranch(const std::shared_ptr<Position>& position, std::weak_ptr<NeuronalComponent> parent)
        : NeuronalComponent(position, parent)
{
    // Additional initialization if needed
}

void DendriteBranch::initialise()
{
    NeuronalComponent::initialise(); // Call base class initialise

    if (!instanceInitialised)
    {
        std::cout << "Initialising DendriteBranch" << std::endl;

        if (onwardDendrites.empty())
        {
            // Create a new Dendrite and connect it
            auto newDendritePosition = std::make_shared<Position>(position->x + 1, position->y + 1, position->z + 1);
            auto newDendrite = std::make_shared<Dendrite>(newDendritePosition, std::static_pointer_cast<NeuronalComponent>(shared_from_this()));
            connectDendrite(newDendrite);

            // Initialise the new Dendrite
            onwardDendrites.back()->initialise();
            onwardDendrites.back()->updateFromDendriteBranch(std::static_pointer_cast<DendriteBranch>(shared_from_this()));
        }

        instanceInitialised = true;
    }
}

void DendriteBranch::connectDendrite(std::shared_ptr<Dendrite> dendrite)
{
    auto coords = get_coordinates(static_cast<int>(onwardDendrites.size() + 1),
                                  static_cast<int>(onwardDendrites.size() + 1), 5);
    auto currentPosition = dendrite->getPosition();
    currentPosition->x += static_cast<double>(std::get<0>(coords));
    currentPosition->y += static_cast<double>(std::get<1>(coords));
    currentPosition->z += static_cast<double>(std::get<2>(coords));
    onwardDendrites.emplace_back(std::move(dendrite));
}

const std::vector<std::shared_ptr<Dendrite>>& DendriteBranch::getDendrites() const
{
    return onwardDendrites;
}

void DendriteBranch::updateFromSoma(std::shared_ptr<Soma> parentSomaPointer)
{
    parentSoma = std::move(parentSomaPointer);
}

std::shared_ptr<Soma> DendriteBranch::getParentSoma() const
{
    return parentSoma;
}

void DendriteBranch::updateFromDendrite(std::shared_ptr<Dendrite> parentDendritePointer)
{
    parentDendrite = std::move(parentDendritePointer);
}

std::shared_ptr<Dendrite> DendriteBranch::getParentDendrite() const
{
    return parentDendrite;
}

void DendriteBranch::update(double deltaTime)
{
    // Update energy levels
    updateEnergy(deltaTime);

    // Update dendrites
    for (auto& dendrite : onwardDendrites)
    {
        dendrite->update(deltaTime);
    }

    // Additional updates if necessary
}

void DendriteBranch::setDendriteBranchId(int id)
{
    dendriteBranchId = id;
}

int DendriteBranch::getDendriteBranchId() const
{
    return dendriteBranchId;
}

