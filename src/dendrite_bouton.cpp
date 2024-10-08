#include "dendrite_bouton.h"

#include "dendrite.h"
#include "neuron.h"
#include "synaptic_gap.h"

#include <utility>

namespace ns_aarnn
{
DendriteBouton::DendriteBouton(const Position& position) : BodyComponent(nextID<DendriteBouton>(), position)
{
    setNextIDFunction(nextID<DendriteBouton>);
}

DendriteBoutonPtr DendriteBouton::create(const Position& position)
{
    return std::shared_ptr<DendriteBouton>(new DendriteBouton(position));
}

std::string_view DendriteBouton::name()
{
    return std::string_view{"DendriteBouton"};
}

void DendriteBouton::doInitialisation_()
{
    // Nothing to do
}

double DendriteBouton::calculatePropagationRate() const
{
    // TODO: default propagation rate should be overridden
    return getPropagationRate();
}

void DendriteBouton::addSynapticGap(SynapticGapPtr gap)
{
    connectParentAndChild(this_shared<DendriteBouton>(), std::move(gap));
    auto parentNeuron =  getParent<Neuron>();
    if(parentNeuron)
        parentNeuron->addSynapticGapDendrite(getChild<SynapticGap>());
    // TODO: throw otherwise
}

void DendriteBouton::setParentNeutron(NeuronPtr parentNeuron)
{
    connectParentAndChild(std::move(parentNeuron), this_shared<DendriteBouton>());
}

void DendriteBouton::setParentDendrite(DendritePtr parentDendrite)
{
    connectParentAndChild(std::move(parentDendrite), this_shared<DendriteBouton>());
}
}  // namespace aarnn
