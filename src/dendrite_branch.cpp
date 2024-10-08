#include "dendrite_branch.h"

#include <utility>
#include "soma.h"
#include "dendrite.h"
#include "position.h"

namespace ns_aarnn
{
DendriteBranch::DendriteBranch(const Position& position) : BodyComponent(nextID<DendriteBranch>(), position)
{
    setNextIDFunction(nextID<DendriteBranch>);
}

DendriteBranchPtr DendriteBranch::create(const Position& position)
{
    return std::shared_ptr<DendriteBranch>(new DendriteBranch(position));
}

std::string_view DendriteBranch::name()
{
    return std::string_view{"DendriteBranch"};
}

void DendriteBranch::doInitialisation_()
{
    if(onwardDendrites_.empty())
    {
        addDendrite(Dendrite::create(Position{x() + 1.0, y() + 1.0, z() + 1.0}));
        onwardDendrites_.back()->initialise();
        onwardDendrites_.back()->setParentDendriteBranch(this_shared<DendriteBranch>());
    }
}

double DendriteBranch::calculatePropagationRate() const
{
    // TODO: default propagation rate should be overridden
    return getPropagationRate();
}

void DendriteBranch::addDendrite(DendritePtr dendrite)
{
    dendrite->move(Position::layeredFibonacciSpherePoint(onwardDendrites_.size() + 1UL, onwardDendrites_.size() + 1UL));
    onwardDendrites_.emplace_back(std::move(dendrite));
}

Dendrites DendriteBranch::getDendrites() const
{
    return onwardDendrites_;
}

void DendriteBranch::setParentSoma(SomaPtr parentSoma)
{
    connectParentAndChild(std::move(parentSoma), this_shared<DendriteBranch>());
 }

void DendriteBranch::setParentDendrite(DendritePtr parentDendrite)
{
    connectParentAndChild(std::move(parentDendrite), this_shared<DendriteBranch>());
}
}  // namespace aarnn
