#include "soma.h"

#include "axon_hillock.h"
#include "dendrite_branch.h"
#include "position.h"

namespace ns_aarnn
{
Soma::Soma(const Position &position) : BodyComponent(nextID<Soma>(), position)
{
    setNextIDFunction(nextID<Soma>);
}

SomaPtr Soma::create(const Position &position)
{
    return std::shared_ptr<Soma>(new Soma(position));
}

std::string_view Soma::name()
{
    return std::string_view{"Soma"};
}

void Soma::doInitialisation_()
{
    if(!onwardAxonHillock_)
    {
        onwardAxonHillock_ = AxonHillock::create(Position{x() + 1.0, y() + 1.0, z() + 1.0});
    }
    onwardAxonHillock_->initialise();
    onwardAxonHillock_->setParentSoma(this_shared<Soma>());
    addDendriteBranch(DendriteBranch::create(Position{x() - 1.0, y() - 1.0, z() - 1.0}));
    dendriteBranches_.back()->initialise();
    dendriteBranches_.back()->setParentSoma(this_shared<Soma>());
}

double Soma::calculatePropagationRate() const
{
    // TODO: default propagation rate should be overridden
    return getPropagationRate();
}

AxonHillockPtr Soma::getAxonHillock() const
{
    return onwardAxonHillock_;
}

void Soma::addDendriteBranch(DendriteBranchPtr dendriteBranch)
{
    dendriteBranch->move(
     Position::layeredFibonacciSpherePoint(dendriteBranches_.size() + 1UL, dendriteBranches_.size() + 1UL));
    dendriteBranches_.emplace_back(std::move(dendriteBranch));
}

const DendriteBranches &Soma::getDendriteBranches() const
{
    return dendriteBranches_;
}

void Soma::setParentNeuron(NeuronPtr parentNeuron)
{
    parentNeuron_ = std::move(parentNeuron);
}

NeuronPtr Soma::getParentNeuron() const
{
    return parentNeuron_;
}

}  // namespace aarnn
