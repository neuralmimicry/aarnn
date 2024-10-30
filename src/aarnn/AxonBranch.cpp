#include "AxonBranch.h"
#include "Axon.h"
#include "utils.h"
#include <iostream>

AxonBranch::AxonBranch(const std::shared_ptr<Position>& position, std::weak_ptr<NeuronalComponent> parent)
        : NeuronalComponent(position, parent)
{
    // Additional initialization if needed
}

void AxonBranch::initialise()
{
    NeuronalComponent::initialise(); // Call base class initialise

    if (!instanceInitialised)
    {
        std::cout << "Initialising AxonBranch" << std::endl;

        if (onwardAxons.empty())
        {
            // Create a new Axon and connect it
            auto newAxonPosition = std::make_shared<Position>(position->x + 1, position->y + 1, position->z + 1);
            auto newAxon = std::make_shared<Axon>(newAxonPosition, std::static_pointer_cast<NeuronalComponent>(shared_from_this()));
            connectAxon(newAxon);

            // Initialise the new Axon
            onwardAxons.back()->initialise();
            onwardAxons.back()->updateFromAxonBranch(std::static_pointer_cast<AxonBranch>(shared_from_this()));
        }

        instanceInitialised = true;
    }
}

void AxonBranch::update(double deltaTime)
{
    // Update energy levels
    updateEnergy(deltaTime);

    // Update onward Axons
    for (auto& axon : onwardAxons)
    {
        axon->update(deltaTime);
    }

    // Additional updates if necessary
}

void AxonBranch::connectAxon(std::shared_ptr<Axon> axon)
{
    auto coords = get_coordinates(static_cast<int>(onwardAxons.size() + 1),
                                  static_cast<int>(onwardAxons.size() + 1), 5);
    auto currentPosition = axon->getPosition();
    currentPosition->x += static_cast<double>(std::get<0>(coords));
    currentPosition->y += static_cast<double>(std::get<1>(coords));
    currentPosition->z += static_cast<double>(std::get<2>(coords));
    onwardAxons.emplace_back(std::move(axon));
}

const std::vector<std::shared_ptr<Axon>>& AxonBranch::getAxons() const
{
    return onwardAxons;
}

void AxonBranch::updateFromAxon(std::shared_ptr<Axon> parentPointer)
{
    parentAxon = std::move(parentPointer);
}

std::shared_ptr<Axon> AxonBranch::getParentAxon() const
{
    return parentAxon;
}

void AxonBranch::setAxonBranchId(int id)
{
    axonBranchId = id;
}

int AxonBranch::getAxonBranchId() const
{
    return axonBranchId;
}

