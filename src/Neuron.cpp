#include "Neuron.h"

std::shared_ptr<Soma> soma;

/**
 * @brief Getter function for the soma of the neuron.
 * @return Shared pointer to the Soma object of the neuron.
 */
std::shared_ptr<Soma> Neuron::getSoma()
{
    return soma;
}
void Neuron::initialise()
{
    if (!instanceInitialised)
    {
        if (!soma)
        {
            soma = std::make_shared<Soma>(std::make_shared<Position>(position->x, position->y, position->z));
        }
        soma->initialise();
        soma->updateFromNeuron(shared_from_this());
        instanceInitialised = true;
    }
}
void Neuron::addSynapticGapDendrite(std::shared_ptr<SynapticGap> synapticGap) { synapticGapsDendrite.emplace_back(std::move(synapticGap)); }

void Neuron::updatePosition(const PositionPtr &newPosition)
{
    position = newPosition;
}

void Neuron::storeAllSynapticGapsAxon()
{
    // Clear the synapticGaps vector before traversing
    synapticGapsAxon.clear();
    axonBoutons.clear();

    // Get the onward axon hillock from the soma
    std::shared_ptr<AxonHillock> onwardAxonHillock = soma->getAxonHillock();
    if (!onwardAxonHillock)
    {
        return;
    }

    // Get the onward axon from the axon hillock
    std::shared_ptr<Axon> onwardAxon = onwardAxonHillock->getAxon();
    if (!onwardAxon)
    {
        return;
    }

    // Recursively traverse the onward axons and their axon boutons to store all synaptic gaps
    traverseAxonsForStorage(onwardAxon);
}

void Neuron::storeAllSynapticGapsDendrite()
{
    // Clear the synapticGaps vector before traversing
    synapticGapsDendrite.clear();
    dendriteBoutons.clear();

    // Recursively traverse the onward dendrite branches, dendrites and their dendrite boutons to store all synaptic gaps
    traverseDendritesForStorage(soma->getDendriteBranches());
}

void Neuron::addSynapticGapAxon(std::shared_ptr<SynapticGap> synapticGap)
{
    synapticGapsAxon.emplace_back(std::move(synapticGap));
}

// Helper function to recursively traverse the axons and find the synaptic gap
std::shared_ptr<SynapticGap> Neuron::traverseAxons(const std::shared_ptr<Axon> &axon, const PositionPtr &positionPtr)
{
    // Check if the current axon's bouton has a gap using the same position pointer
    std::shared_ptr<AxonBouton> axonBouton = axon->getAxonBouton();
    if (axonBouton)
    {
        std::shared_ptr<SynapticGap> gap = axonBouton->getSynapticGap();
        if (gap->getPosition() == positionPtr)
        {
            return gap;
        }
    }

    // Recursively traverse the axon's branches and onward axons
    const std::vector<std::shared_ptr<AxonBranch>> &branches = axon->getAxonBranches();
    for (const auto &branch : branches)
    {
        const std::vector<std::shared_ptr<Axon>> &onwardAxons = branch->getAxons();
        for (const auto &onwardAxon : onwardAxons)
        {
            std::shared_ptr<SynapticGap> gap = traverseAxons(onwardAxon, positionPtr);
            if (gap)
            {
                return gap;
            }
        }
    }

    return nullptr;
}

// Helper function to recursively traverse the dendrites and find the synaptic gap
std::shared_ptr<SynapticGap> Neuron::traverseDendrites(const std::shared_ptr<Dendrite> &dendrite, const PositionPtr &positionPtr)
{
    // Check if the current dendrite's bouton is near the same position pointer
    std::shared_ptr<DendriteBouton> dendriteBouton = dendrite->getDendriteBouton();
    if (dendriteBouton)
    {
        std::shared_ptr<SynapticGap> gap = dendriteBouton->getSynapticGap();
        if (gap->getPosition() == positionPtr)
        {
            return gap;
        }
    }

    // Recursively traverse the dendrite's branches and onward dendrites
    const std::vector<std::shared_ptr<DendriteBranch>> &dendriteBranches = dendrite->getDendriteBranches();
    for (const auto &dendriteBranch : dendriteBranches)
    {
        const std::vector<std::shared_ptr<Dendrite>> &childDendrites = dendriteBranch->getDendrites();
        for (const auto &onwardDendrites : childDendrites)
        {
            std::shared_ptr<SynapticGap> gap = traverseDendrites(onwardDendrites, positionPtr);
            if (gap)
            {
                return gap;
            }
        }
    }

    return nullptr;
}

// Helper function to recursively traverse the axons and store all synaptic gaps
void Neuron::traverseAxonsForStorage(const std::shared_ptr<Axon> &axon)
{
    // Check if the current axon's bouton has a gap and store it
    std::shared_ptr<AxonBouton> axonBouton = axon->getAxonBouton();
    if (axonBouton)
    {
        std::shared_ptr<SynapticGap> gap = axonBouton->getSynapticGap();
        synapticGapsAxon.emplace_back(std::move(gap));
    }

    // Recursively traverse the axon's branches and onward axons
    const std::vector<std::shared_ptr<AxonBranch>> &branches = axon->getAxonBranches();
    for (const auto &branch : branches)
    {
        const std::vector<std::shared_ptr<Axon>> &onwardAxons = branch->getAxons();
        for (const auto &onwardAxon : onwardAxons)
        {
            traverseAxonsForStorage(onwardAxon);
        }
    }
}

// Helper function to recursively traverse the dendrites and store all dendrite boutons
void Neuron::traverseDendritesForStorage(const std::vector<std::shared_ptr<DendriteBranch>> &dendriteBranches)
{
    for (const auto &branch : dendriteBranches)
    {
        const std::vector<std::shared_ptr<Dendrite>> &onwardDendrites = branch->getDendrites();
        for (const auto &onwardDendrite : onwardDendrites)
        {
            // Check if the current dendrite's bouton has a gap and store it
            std::shared_ptr<DendriteBouton> dendriteBouton = onwardDendrite->getDendriteBouton();
            std::shared_ptr<SynapticGap> gap = dendriteBouton->getSynapticGap();
            if (gap)
            {
                synapticGapsDendrite.emplace_back(gap);
            }
            traverseDendritesForStorage(onwardDendrite->getDendriteBranches());
        }
    }
}
