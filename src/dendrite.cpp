#include "dendrite.h"

#include <utility>

#include "dendrite_bouton.h"
#include "dendrite_branch.h"
#include "position.h"

namespace ns_aarnn
{
Dendrite::Dendrite(const Position& position) : BodyComponent(nextID<Dendrite>(), position)
{
    setNextIDFunction(nextID<Dendrite>);
}

DendritePtr Dendrite::create(Position position)
{
    return std::shared_ptr<Dendrite>(new Dendrite(position));
}

std::string_view Dendrite::name()
{
    return std::string_view{"Dendrite"};
}

void Dendrite::doInitialisation_()
{
    auto dendriteBouton = DendriteBouton::create(Position{x() + 1.0, y() + 1.0, z() + 1.0});
    dendriteBouton->initialise();
    connectParentAndChild(this_shared<Dendrite>(), dendriteBouton);
}

double Dendrite::calculatePropagationRate() const
{
    // TODO: default propagation rate should be overridden
    return getPropagationRate();
}

const DendriteBranches& Dendrite::getDendriteBranches() const
{
    return dendriteBranches_;
}

void Dendrite::setParentDendriteBranch(DendriteBranchPtr parentDendriteBranch)
{
    connectParentAndChild(std::move(parentDendriteBranch), this_shared<Dendrite>());
}

void Dendrite::addBranch(DendriteBranchPtr branch)
{
    branch->move(Position::layeredFibonacciSpherePoint(dendriteBranches_.size() + 1UL, dendriteBranches_.size() + 1UL));
    dendriteBranches_.emplace_back(std::move(branch));
}

}  // namespace aarnn
