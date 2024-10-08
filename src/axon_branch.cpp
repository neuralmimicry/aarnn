#include "axon_branch.h"

#include <utility>

#include "position.h"

namespace ns_aarnn
{
AxonBranch::AxonBranch(const Position &position) : BodyComponent(nextID<AxonBranch>(), position)
{
    setNextIDFunction(nextID<AxonBranch>);
}

AxonBranchPtr AxonBranch::create(const Position &position)
{
    return std::shared_ptr<AxonBranch>(new AxonBranch(position));
}

std::string_view AxonBranch::name()
{
    return std::string_view{"AxonBranch"};
}

void AxonBranch::doInitialisation_()
{
    if(onwardAxons_.empty())
    {
        addAxon(Axon::create(Position{x() + 1.0, y() + 1.0, z() + 1.0}));
        onwardAxons_.back()->initialise();
        onwardAxons_.back()->setParentAxonBranch(this_shared<AxonBranch>());
    }
}

double AxonBranch::calculatePropagationRate() const
{
    // TODO: default propagation rate should be overridden
    return getPropagationRate();
}

void AxonBranch::addAxon(AxonPtr axon)
{
    const auto positionOnFiboSphere =
     Position::layeredFibonacciSpherePoint(onwardAxons_.size() + 1UL, onwardAxons_.size() + 1UL);
    axon->move(positionOnFiboSphere);
    onwardAxons_.emplace_back(std::move(axon));
}

void AxonBranch::setParentAxon(AxonPtr parentAxon)
{
    connectParentAndChild(std::move(parentAxon), this_shared<AxonBranch>());
}

const Axons &AxonBranch::getAxons() const
{
    return onwardAxons_;
}
}  // namespace aarnn
