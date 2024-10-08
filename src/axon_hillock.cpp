#include "axon_hillock.h"

#include <utility>
#include "soma.h"

#include "axon.h"

namespace ns_aarnn
{
AxonHillock::AxonHillock(const Position &position) : BodyComponent(nextID<AxonHillock>(), position)
{
    setNextIDFunction(nextID<AxonHillock>);
}

AxonHillockPtr AxonHillock::create(const Position &position)
{
    return std::shared_ptr<AxonHillock>(new AxonHillock(position));
}

std::string_view AxonHillock::name()
{
    return std::string_view{"AxonHillock"};
}

void AxonHillock::doInitialisation_()
{
    auto onwardAxon = Axon::create(Position{x() + 1.0, y() + 1.0, z() + 1.0});
    onwardAxon->initialise();
    connectParentAndChild(this_shared<AxonHillock>(), onwardAxon);
}

double AxonHillock::calculatePropagationRate() const
{
    // TODO: default propagation rate should be overridden
    return getPropagationRate();
}
void AxonHillock::setParentSoma(SomaPtr parentSoma)
{
    connectParentAndChild(std::move(parentSoma), this_shared<AxonHillock>());
}
}  // namespace aarnn
