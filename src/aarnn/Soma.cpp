#include "Soma.h"
#include "AxonHillock.h"
#include "DendriteBranch.h"
#include "Neuron.h"
#include "utils.h"
#include <iostream>

Soma::Soma(const std::shared_ptr<Position>& position)
        : NeuronalComponent(position)
{
    // Additional initialization if needed
}

void Soma::initialise()
{
    NeuronalComponent::initialise(); // Call base class initialise

    if (!instanceInitialised)
    {
        std::cout << "Initialising Soma" << std::endl;

        if (!onwardAxonHillock)
        {
            std::cout << "Creating AxonHillock" << std::endl;
            onwardAxonHillock = std::make_shared<AxonHillock>(
                    std::make_shared<Position>(position->x + 1, position->y + 1, position->z + 1));
        }
        std::cout << "Soma initialising AxonHillock" << std::endl;
        onwardAxonHillock->initialise();
        std::cout << "Soma updating from AxonHillock" << std::endl;
        onwardAxonHillock->updateFromSoma(std::static_pointer_cast<Soma>(shared_from_this()));

        std::cout << "Creating DendriteBranch" << std::endl;
        auto dendriteBranch = std::make_shared<DendriteBranch>(
                std::make_shared<Position>(position->x - 1, position->y - 1, position->z - 1));
        addDendriteBranch(dendriteBranch);

        std::cout << "Soma initialising DendriteBranch" << std::endl;
        dendriteBranches.back()->initialise();
        std::cout << "Soma updating from DendriteBranch" << std::endl;
        dendriteBranches.back()->updateFromSoma(std::static_pointer_cast<Soma>(shared_from_this()));

        instanceInitialised = true;
    }
    std::cout << "Soma initialised" << std::endl;
}

void Soma::update(double deltaTime)
{
    // Update energy levels
    updateEnergy(deltaTime);

    // Update the axon hillock
    if (onwardAxonHillock)
    {
        onwardAxonHillock->update(deltaTime);
    }

    // Update dendrite branches
    for (auto& onwardDendriteBranch : dendriteBranches)
    {
        onwardDendriteBranch->update(deltaTime);
    }

    // Additional updates if necessary
}

std::shared_ptr<AxonHillock> Soma::getAxonHillock() const
{
    return onwardAxonHillock;
}

void Soma::addDendriteBranch(std::shared_ptr<DendriteBranch> dendriteBranch)
{
    auto coords = get_coordinates(static_cast<int>(dendriteBranches.size() + 1),
                                  static_cast<int>(dendriteBranches.size() + 1), 5);
    auto currentPosition = dendriteBranch->getPosition();
    currentPosition->x += static_cast<double>(std::get<0>(coords));
    currentPosition->y += static_cast<double>(std::get<1>(coords));
    currentPosition->z += static_cast<double>(std::get<2>(coords));
    dendriteBranches.emplace_back(std::move(dendriteBranch));
}

const std::vector<std::shared_ptr<DendriteBranch>>& Soma::getDendriteBranches() const
{
    return dendriteBranches;
}

void Soma::updateFromNeuron(std::shared_ptr<Neuron> parentPointer)
{
    parentNeuron = std::move(parentPointer);
}

std::shared_ptr<Neuron> Soma::getParentNeuron() const
{
    return parentNeuron;
}

void Soma::setSomaId(int id)
{
    somaId = id;
}

int Soma::getSomaId() const
{
    return somaId;
}
