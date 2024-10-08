#include "axon.h"

#include "axon_bouton.h"
#include "axon_branch.h"
#include "axon_hillock.h"
#include "body_component.h"

#include <utility>

namespace ns_aarnn
{
Axon::Axon(const Position& position) : BodyComponent(nextID<Axon>(), position)
{
    setNextIDFunction(nextID<Axon>);
}

AxonPtr Axon::create(const Position& position)
{
    return std::shared_ptr<Axon>(new Axon(position));
}

std::string_view Axon::name()
{
    return std::string_view{"Axon"};
}

void Axon::doInitialisation_()
{
    auto onwardAxonBouton = AxonBouton::create(Position{x() + 1.0, y() + 1.0, z() + 1.0});
    onwardAxonBouton->initialise();
    connectParentAndChild(this_shared<Axon>(), std::move(onwardAxonBouton));
}

double Axon::calculatePropagationRate() const
{
    // TODO: default propagation rate should be overridden
    return getPropagationRate();
}

void Axon::addBranch(AxonBranchPtr branch)
{
    axonBranches_.push_back(std::move(branch));
}

const AxonBranches& Axon::getAxonBranches() const
{
    return axonBranches_;
}

void Axon::setParentAxonHillock(AxonHillockPtr parentAxonHillock)
{
    connectParentAndChild(std::move(parentAxonHillock), this_shared<Axon>());
}

void Axon::setParentAxonBranch(AxonBranchPtr parentAxonBranch)
{
    connectParentAndChild(std::move(parentAxonBranch), this_shared<Axon>());
}

}  // namespace aarnn