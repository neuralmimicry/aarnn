#include "effector.h"

namespace ns_aarnn
{
Effector::Effector(Position position) : BodyComponent(nextID<Effector>(), position)
{
    setNextIDFunction(nextID<Effector>);
}

EffectorPtr Effector::create(Position position)
{
    return std::shared_ptr<Effector>(new Effector(position));
}

std::string_view Effector::name()
{
    return std::string_view{"Effector"};
}

void Effector::doInitialisation_()
{
    // Nothing to do
}

double Effector::calculatePropagationRate() const
{
    // TODO: default propagation rate should be overridden
    return getPropagationRate();
}

SynapticGaps Effector::getSynapticGaps() const
{
    return synapticGaps_;
}

void Effector::addSynapticGap(SynapticGapPtr synapticGap)
{
    synapticGaps_.emplace_back(std::move(synapticGap));
}

}  // namespace aarnn
