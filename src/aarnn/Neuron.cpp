#include "Neuron.h"
#include "Soma.h"
#include <iostream>

// Initialize static member
int Neuron::nextNeuronId = 0;

// Constructor
Neuron::Neuron(const std::shared_ptr<Position>& position)
        : NeuronalComponent(position), neuronId(nextNeuronId++)
{
}

// Get the soma associated with the neuron
std::shared_ptr<Soma> Neuron::getSoma()
{
    return this->soma;
}

// Initialize the neuron
void Neuron::initialise()
{
    NeuronalComponent::initialise(); // Call base class initialise

    if (!instanceInitialised)
    {
        std::cout << "Initialising Neuron" << std::endl;

        if (!this->soma)
        {
            std::cout << "Creating Soma" << std::endl;
            // Pass the neuron's position to the soma
            this->soma = std::make_shared<Soma>(position);
        }

        std::cout << "Neuron initialising Soma" << std::endl;
        this->soma->initialise();
        std::cout << "Neuron updating Soma" << std::endl;
        this->soma->updateFromNeuron(std::static_pointer_cast<Neuron>(shared_from_this()));

        instanceInitialised = true; // Mark as initialized
    }

    std::cout << "Neuron initialised" << std::endl;
}

// Add a synaptic gap to the dendrite list
void Neuron::addSynapticGapDendrite(std::shared_ptr<SynapticGap> synapticGap)
{
    synapticGapsDendrite.emplace_back(std::move(synapticGap));
}

// Store all synaptic gaps from the axon
void Neuron::storeAllSynapticGapsAxon()
{
    synapticGapsAxon.clear();
    axonBoutons.clear();

    if (!soma)
    {
        return;
    }

    std::shared_ptr<AxonHillock> onwardAxonHillock = this->soma->getAxonHillock();
    if (!onwardAxonHillock)
    {
        return;
    }

    std::shared_ptr<Axon> onwardAxon = onwardAxonHillock->getAxon();
    if (!onwardAxon)
    {
        return;
    }

    traverseAxonsForStorage(onwardAxon);
}

// Store all synaptic gaps from the dendrite
void Neuron::storeAllSynapticGapsDendrite()
{
    synapticGapsDendrite.clear();
    dendriteBoutons.clear();

    if (!soma)
    {
        return;
    }

    traverseDendritesForStorage(this->soma->getDendriteBranches());
}

// Add a synaptic gap to the axon list
void Neuron::addSynapticGapAxon(std::shared_ptr<SynapticGap> synapticGap)
{
    synapticGapsAxon.emplace_back(std::move(synapticGap));
}

// Traverse axons to find a specific synaptic gap
std::shared_ptr<SynapticGap> Neuron::traverseAxons(const std::shared_ptr<Axon>& axon, const std::shared_ptr<Position>& positionPtr)
{
    std::shared_ptr<AxonBouton> axonBouton = axon->getAxonBouton();
    if (axonBouton)
    {
        std::shared_ptr<SynapticGap> gap = axonBouton->getSynapticGap();
        if (gap && gap->getPosition() == positionPtr)
        {
            return gap;
        }
    }

    const std::vector<std::shared_ptr<AxonBranch>>& branches = axon->getAxonBranches();
    for (const auto& branch : branches)
    {
        const std::vector<std::shared_ptr<Axon>>& onwardAxons = branch->getAxons();
        for (const auto& onwardAxon : onwardAxons)
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

// Traverse dendrites to find a specific synaptic gap
std::shared_ptr<SynapticGap> Neuron::traverseDendrites(const std::shared_ptr<Dendrite>& dendrite, const std::shared_ptr<Position>& positionPtr)
{
    std::shared_ptr<DendriteBouton> dendriteBouton = dendrite->getDendriteBouton();
    if (dendriteBouton)
    {
        std::shared_ptr<SynapticGap> gap = dendriteBouton->getSynapticGap();
        if (gap && gap->getPosition() == positionPtr)
        {
            return gap;
        }
    }

    const std::vector<std::shared_ptr<DendriteBranch>>& dendriteBranches = dendrite->getDendriteBranches();
    for (const auto& branch : dendriteBranches)
    {
        const std::vector<std::shared_ptr<Dendrite>>& childDendrites = branch->getDendrites();
        for (const auto& onwardDendrite : childDendrites)
        {
            std::shared_ptr<SynapticGap> gap = traverseDendrites(onwardDendrite, positionPtr);
            if (gap)
            {
                return gap;
            }
        }
    }

    return nullptr;
}

// Traverse axons to store synaptic gaps
void Neuron::traverseAxonsForStorage(const std::shared_ptr<Axon>& axon)
{
    std::shared_ptr<AxonBouton> axonBouton = axon->getAxonBouton();
    if (axonBouton)
    {
        std::shared_ptr<SynapticGap> gap = axonBouton->getSynapticGap();
        if (gap)
        {
            synapticGapsAxon.emplace_back(std::move(gap));
        }
    }

    const std::vector<std::shared_ptr<AxonBranch>>& branches = axon->getAxonBranches();
    for (const auto& branch : branches)
    {
        const std::vector<std::shared_ptr<Axon>>& onwardAxons = branch->getAxons();
        for (const auto& onwardAxon : onwardAxons)
        {
            traverseAxonsForStorage(onwardAxon);
        }
    }
}

// Traverse dendrites to store synaptic gaps
void Neuron::traverseDendritesForStorage(const std::vector<std::shared_ptr<DendriteBranch>>& dendriteBranches)
{
    for (const auto& branch : dendriteBranches)
    {
        const std::vector<std::shared_ptr<Dendrite>>& onwardDendrites = branch->getDendrites();
        for (const auto& onwardDendrite : onwardDendrites)
        {
            std::shared_ptr<DendriteBouton> dendriteBouton = onwardDendrite->getDendriteBouton();
            if (dendriteBouton)
            {
                std::shared_ptr<SynapticGap> gap = dendriteBouton->getSynapticGap();
                if (gap)
                {
                    synapticGapsDendrite.emplace_back(std::move(gap));
                }
            }

            traverseDendritesForStorage(onwardDendrite->getDendriteBranches());
        }
    }
}

// Get synaptic gaps from the axon
std::vector<std::shared_ptr<SynapticGap>> Neuron::getSynapticGapsAxon() const
{
    return synapticGapsAxon;
}

// Get dendrite boutons
std::vector<std::shared_ptr<DendriteBouton>> Neuron::getDendriteBoutons() const
{
    return dendriteBoutons;
}

// Set the propagation rate
void Neuron::setPropagationRate(double rate)
{
    propagationRate = rate;
}

// Get the propagation rate
double Neuron::getPropagationRate() const
{
    return propagationRate;
}

// Set the neuron ID
void Neuron::setNeuronId(int id)
{
    neuronId = id;
}

// Get the neuron ID
int Neuron::getNeuronId() const
{
    return neuronId;
}

// Set the neuron type
void Neuron::setNeuronType(int type)
{
    neuronType = type;
}

// Get the neuron type
int Neuron::getNeuronType() const
{
    return neuronType;
}

// Update the neuron state over time
void Neuron::update(double deltaTime)
{
    // Update energy levels
    updateEnergy(deltaTime);

    // Update the soma
    if (this->soma)
    {
        this->soma->update(deltaTime);
    }

    // Additional updates if necessary
}

void Neuron::updateFromCluster(std::shared_ptr<Cluster> parentPointer)
{
    parentCluster = std::move(parentPointer);
}

std::shared_ptr<Cluster> Neuron::getParentCluster() const
{
    return parentCluster;
}
