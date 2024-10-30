#include "Dendrite.h"
#include "DendriteBouton.h"
#include "Position.h"
#include <memory>

Dendrite::Dendrite(const std::shared_ptr<Position>& position, std::weak_ptr<NeuronalComponent> parent)
        : NeuronalComponent(position, parent)
{
    // Additional initialization if needed
}

void Dendrite::initialise()
{
    NeuronalComponent::initialise(); // Call base class initialise

    if (!instanceInitialised)
    {
        if (!this->dendriteBouton)
        {
            this->dendriteBouton = std::make_shared<DendriteBouton>(
                    std::make_shared<Position>(position->x - 1, position->y - 1, position->z - 1), std::static_pointer_cast<NeuronalComponent>(shared_from_this()));
            this->dendriteBouton->initialise();
            this->dendriteBouton->updateFromDendrite(std::static_pointer_cast<Dendrite>(shared_from_this()));
        }
        instanceInitialised = true;
    }
}

void Dendrite::addBranch(std::shared_ptr<DendriteBranch> branch)
{
    dendriteBranches.emplace_back(std::move(branch));
}

const std::vector<std::shared_ptr<DendriteBranch>>& Dendrite::getDendriteBranches() const
{
    return dendriteBranches;
}

std::shared_ptr<DendriteBouton> Dendrite::getDendriteBouton() const
{
    return dendriteBouton;
}

void Dendrite::updateFromDendriteBranch(std::shared_ptr<DendriteBranch> parentDendriteBranchPointer)
{
    parentDendriteBranch = std::move(parentDendriteBranchPointer);
}

std::shared_ptr<DendriteBranch> Dendrite::getParentDendriteBranch() const
{
    return parentDendriteBranch;
}

std::shared_ptr<DendriteBouton> Dendrite::getDendriteBoutons()
{
    return dendriteBouton;
}

void Dendrite::update(double deltaTime)
{
    // Update energy levels
    updateEnergy(deltaTime);

    // Update the axon hillock
    if (dendriteBouton)
    {
        dendriteBouton->update(deltaTime);
    }

    // Additional updates if necessary
}

void Dendrite::setDendriteId(int id)
{
    dendriteId = id;
}

int Dendrite::getDendriteId() const
{
    return dendriteId;
}


