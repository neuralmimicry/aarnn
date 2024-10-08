#include "axon.h"
#include "axon_bouton.h"
#include "neuron.h"

#include "synaptic_gap.h"

namespace ns_aarnn
{
AxonBouton::AxonBouton(const Position& position) : BodyComponent(nextID<AxonBouton>(), position)
{
    setNextIDFunction(nextID<AxonBouton>);
}

AxonBoutonPtr AxonBouton::create(const Position& position)
{
    return std::shared_ptr<AxonBouton>(new AxonBouton(position));
}

std::string_view AxonBouton::name()
{
    return std::string_view{"AxonBouton"};
}

void AxonBouton::doInitialisation_()
{
     auto onwardSynapticGap = SynapticGap::create(Position{x() + 1.0, y() + 1.0, z() + 1.0});
     onwardSynapticGap->initialise();
     connectParentAndChild(this_shared<AxonBouton>(), onwardSynapticGap);
}

double AxonBouton::calculatePropagationRate() const
{
    // TODO: default propagation rate should be overridden
    return getPropagationRate();
}

void AxonBouton::setParentNeuron(NeuronPtr parentNeuron)
{
    connectParentAndChild(std::move(parentNeuron), this_shared<AxonBouton>());
}

void AxonBouton::setParentAxon(AxonPtr parentAxon)
{
    connectParentAndChild(std::move(parentAxon), this_shared<AxonBouton>());
}

}  // namespace aarnn
