#include "neuron.h"

#include "axon.h"
#include "axon_bouton.h"
#include "axon_branch.h"
#include "axon_hillock.h"
#include "dendrite.h"
#include "dendrite_bouton.h"
#include "dendrite_branch.h"
#include "soma.h"
#include "synaptic_gap.h"

namespace ns_aarnn
{
Neuron::Neuron(Position position) : BodyComponent(nextID<Neuron>(), position)
{
    setNextIDFunction(nextID<Neuron>);
}

NeuronPtr Neuron::create(const Position &position)
{
    return std::shared_ptr<Neuron>(new Neuron(position));
}

std::string_view Neuron::name()
{
    return std::string_view{"Neuron"};
}

void Neuron::doInitialisation_()
{
    auto soma = Soma::create(Position{x(), y(), z()});
    soma->initialise();
    connectParentAndChild(this_shared<Neuron>(), soma);
}

double Neuron::calculatePropagationRate() const
{
    return getChild<Soma>()->calculatePropagationRate();
}

SynapticGaps &Neuron::getSynapticGapsAxon()
{
    return synapticGaps_Axon_;
}

DendriteBoutons &Neuron::getDendriteBoutons()
{
    return dendriteBoutons_;
}

void Neuron::addSynapticGapDendrite(SynapticGapPtr synapticGap)
{
    synapticGaps_Dendrite_.emplace_back(std::move(synapticGap));
}

void Neuron::storeAllSynapticGapsAxon()
{
    // Clear the synapticGaps_ vector before traversing
    synapticGaps_Axon_.clear();
    axonBoutons_.clear();

    // Get the onward axon hillock from the soma
    AxonHillockPtr onwardAxonHillock = getChild<Soma>()->getAxonHillock();
    if(!onwardAxonHillock)
    {
        return;
    }

    // Get the onward axon from the axon hillock
    AxonPtr onwardAxon_ = onwardAxonHillock->getChild<Axon>();
    if(!onwardAxon_)
    {
        return;
    }

    // Recursively traverse the onward axons and their axon boutons to store all synaptic gaps
    traverseAxonsForStorage(onwardAxon_);
}

void Neuron::storeAllSynapticGapsDendrite()
{
    // Clear the synapticGaps_ vector before traversing
    synapticGaps_Dendrite_.clear();
    dendriteBoutons_.clear();

    // Recursively traverse the onward dendrite branches, dendrites and their dendrite boutons to store all synaptic
    // gaps
    traverseDendritesForStorage(getChild<Soma>()->getDendriteBranches());
}

void Neuron::addSynapticGapAxon(SynapticGapPtr synapticGap)
{
    synapticGaps_Axon_.emplace_back(std::move(synapticGap));
}

// Helper function to recursively traverse the axons and find the synaptic gap
SynapticGapPtr findSynapticGap(const Axon &axon, const Position &position)
{
    // Check if the current axon's bouton has a gap using the same position pointer
    AxonBoutonPtr axonBouton = axon.getChild<AxonBouton>();
    if(axonBouton)
    {
        SynapticGapPtr gap = axonBouton->getChild<SynapticGap>();
        if(static_cast<Position>(*gap) == position)
        {
            return gap;
        }
    }

    // Recursively traverse the axon's branches and onward axons
    const AxonBranches &branches = axon.getAxonBranches();
    for(const auto &branch: branches)
    {
        const Axons &onwardAxons_ = branch->getAxons();
        for(const auto &onwardAxon: onwardAxons_)
        {
            SynapticGapPtr gap = findSynapticGap(*onwardAxon, position);
            if(gap)
            {
                return gap;
            }
        }
    }

    return nullptr;
}

void Neuron::computePropagationRate(std::atomic<double> &totalPropagationRate) const
{
    // Get the propagation rate of the neuron from the DendriteBouton to the SynapticGap
    // Lock the mutex to safely update the totalPropagationRate
    {
        std::lock_guard<std::mutex> lock(mtx());
        totalPropagationRate = totalPropagationRate + calculatePropagationRate();
    }
}

void Neuron::associateSynapticGap(const NeuronPtr &neuron2, double proximityThreshold)
{
    // Iterate over all SynapticGaps in neuron1
    for(auto &gap: getSynapticGapsAxon())
    {
        // If the bouton has a synaptic gap, and it is not associated_ yet
        if(gap && !gap->isAssociated())
        {
            // Iterate over all DendriteBoutons in neuron2
            for(auto &dendriteBouton: neuron2->getDendriteBoutons())
            {
                spdlog::info("Checking gap {} with dendrite bouton {}", toString(gap), toString(dendriteBouton));
                // If the distance between the bouton and the dendriteBouton_ is below the proximity threshold
                if(static_cast<Position>(*gap).distanceTo(*dendriteBouton) < proximityThreshold)
                {
                    // Associate the synaptic gap with the dendriteBouton_
                    dendriteBouton->addSynapticGap(gap);
                    // Set the SynapticGap as associated_
                    gap->setAsAssociated();
                    spdlog::info("Associated gap {} with dendrite bouton {}", toString(gap), toString(dendriteBouton));
                    // No need to check other DendriteBoutons once the gap is associated_
                    break;
                }
            }
        }
    }
}

// Helper function to recursively traverse the dendrites and find the synaptic gap
SynapticGapPtr findSynapticGap(const Dendrite &dendrite, const Position &position)
{
    // Check if the current dendrite's bouton is near the same position pointer
    const DendriteBoutonPtr &dendriteBouton = dendrite.getChild<DendriteBouton>();
    if(dendriteBouton)
    {
        SynapticGapPtr gap = dendriteBouton->getChild<SynapticGap>();
        if(static_cast<Position>(*gap) == position)
        {
            return gap;
        }
    }

    // Recursively traverse the dendrite's branches and onward dendrites
    const DendriteBranches &dendriteBranches = dendrite.getDendriteBranches();
    for(const auto &dendriteBranch: dendriteBranches)
    {
        const Dendrites &childDendrites = dendriteBranch->getDendrites();
        for(const auto &onwardDendrite: childDendrites)
        {
            SynapticGapPtr gap = findSynapticGap(*onwardDendrite, position);
            if(gap)
            {
                return gap;
            }
        }
    }

    return nullptr;
}

// Helper function to recursively traverse the axons and store all synaptic gaps
void Neuron::traverseAxonsForStorage(const AxonPtr &axon)
{
    // Check if the current axon's bouton has a gap and store it
    AxonBoutonPtr axonBouton = axon->getChild<AxonBouton>();
    if(axonBouton)
    {
        SynapticGapPtr gap = axonBouton->getChild<SynapticGap>();
        synapticGaps_Axon_.emplace_back(std::move(gap));
    }

    // Recursively traverse the axon's branches and onward axons
    const AxonBranches &branches = axon->getAxonBranches();
    for(const auto &branch: branches)
    {
        const Axons &onwardAxons_ = branch->getAxons();
        for(const auto &onwardAxon: onwardAxons_)
        {
            traverseAxonsForStorage(onwardAxon);
        }
    }
}

// Helper function to recursively traverse the dendrites and store all dendrite boutons
void Neuron::traverseDendritesForStorage(const DendriteBranches &dendriteBranches)
{
    for(const auto &branch: dendriteBranches)
    {
        const Dendrites &onwardDendrites_ = branch->getDendrites();
        for(const auto &onwardDendrite: onwardDendrites_)
        {
            // Check if the current dendrite's bouton has a gap and store it
            const DendriteBoutonPtr &dendriteBouton = onwardDendrite->getChild<DendriteBouton>();
            SynapticGapPtr           gap            = dendriteBouton->getChild<SynapticGap>();
            if(gap)
            {
                synapticGaps_Dendrite_.emplace_back(gap);
            }
            traverseDendritesForStorage(onwardDendrite->getDendriteBranches());
        }
    }
}
}  // namespace aarnn
