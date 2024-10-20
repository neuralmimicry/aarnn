#include "Neuron.h"

int Neuron::nextNeuronId = 0;

// std::shared_ptr<Soma> soma;

std::shared_ptr<Soma> Neuron::getSoma() {
    return this->soma;
}

void Neuron::initialise() {
    std::cout << "Initialising Neuron" << std::endl;
    if (!instanceInitialised) {
        if (!this->soma) {
            std::cout << "Creating Soma" << std::endl;
            this->soma = std::make_shared<Soma>(std::make_shared<Position>(position->x, position->y, position->z));
        }
        std::cout << "Neuron initialising Soma" << std::endl;
        this->soma->initialise();
        std::cout << "Neuron updating Soma" << std::endl;
        this->soma->updateFromNeuron(shared_from_this());
        instanceInitialised = true;
    }
    std::cout << "Neuron initialised" << std::endl;
}

void Neuron::addSynapticGapDendrite(std::shared_ptr<SynapticGap> synapticGap) {
    synapticGapsDendrite.emplace_back(std::move(synapticGap));
}

void Neuron::storeAllSynapticGapsAxon() {
    synapticGapsAxon.clear();
    axonBoutons.clear();
    std::shared_ptr<AxonHillock> onwardAxonHillock = this->soma->getAxonHillock();
    if (!onwardAxonHillock) {
        return;
    }
    std::shared_ptr<Axon> onwardAxon = onwardAxonHillock->getAxon();
    if (!onwardAxon) {
        return;
    }
    traverseAxonsForStorage(onwardAxon);
}

void Neuron::storeAllSynapticGapsDendrite() {
    synapticGapsDendrite.clear();
    dendriteBoutons.clear();
    traverseDendritesForStorage(this->soma->getDendriteBranches());
}

void Neuron::addSynapticGapAxon(std::shared_ptr<SynapticGap> synapticGap) {
    synapticGapsAxon.emplace_back(std::move(synapticGap));
}

std::shared_ptr<SynapticGap> Neuron::traverseAxons(const std::shared_ptr<Axon> &axon, const std::shared_ptr<Position> &positionPtr) {
    std::shared_ptr<AxonBouton> axonBouton = axon->getAxonBouton();
    if (axonBouton) {
        std::shared_ptr<SynapticGap> gap = axonBouton->getSynapticGap();
        if (gap->getPosition() == positionPtr) {
            return gap;
        }
    }
    const std::vector<std::shared_ptr<AxonBranch>> &branches = axon->getAxonBranches();
    for (const auto &branch : branches) {
        const std::vector<std::shared_ptr<Axon>> &onwardAxons = branch->getAxons();
        for (const auto &onwardAxon : onwardAxons) {
            std::shared_ptr<SynapticGap> gap = traverseAxons(onwardAxon, positionPtr);
            if (gap) {
                return gap;
            }
        }
    }
    return nullptr;
}

std::shared_ptr<SynapticGap> Neuron::traverseDendrites(const std::shared_ptr<Dendrite> &dendrite, const std::shared_ptr<Position> &positionPtr) {
    std::shared_ptr<DendriteBouton> dendriteBouton = dendrite->getDendriteBouton();
    if (dendriteBouton) {
        std::shared_ptr<SynapticGap> gap = dendriteBouton->getSynapticGap();
        if (gap->getPosition() == positionPtr) {
            return gap;
        }
    }
    const std::vector<std::shared_ptr<DendriteBranch>> &dendriteBranches = dendrite->getDendriteBranches();
    for (const auto &branch : dendriteBranches) {
        const std::vector<std::shared_ptr<Dendrite>> &childDendrites = branch->getDendrites();
        for (const auto &onwardDendrites : childDendrites) {
            std::shared_ptr<SynapticGap> gap = traverseDendrites(onwardDendrites, positionPtr);
            if (gap) {
                return gap;
            }
        }
    }
    return nullptr;
}

void Neuron::traverseAxonsForStorage(const std::shared_ptr<Axon> &axon) {
    std::shared_ptr<AxonBouton> axonBouton = axon->getAxonBouton();
    if (axonBouton) {
        std::shared_ptr<SynapticGap> gap = axonBouton->getSynapticGap();
        synapticGapsAxon.emplace_back(std::move(gap));
    }
    const std::vector<std::shared_ptr<AxonBranch>> &branches = axon->getAxonBranches();
    for (const auto &branch : branches) {
        const std::vector<std::shared_ptr<Axon>> &onwardAxons = branch->getAxons();
        for (const auto &onwardAxon : onwardAxons) {
            traverseAxonsForStorage(onwardAxon);
        }
    }
}

void Neuron::traverseDendritesForStorage(const std::vector<std::shared_ptr<DendriteBranch>> &dendriteBranches) {
    for (const auto &branch : dendriteBranches) {
        const std::vector<std::shared_ptr<Dendrite>> &onwardDendrites = branch->getDendrites();
        for (const auto &onwardDendrite : onwardDendrites) {
            std::shared_ptr<DendriteBouton> dendriteBouton = onwardDendrite->getDendriteBouton();
            std::shared_ptr<SynapticGap> gap = dendriteBouton->getSynapticGap();
            if (gap) {
                synapticGapsDendrite.emplace_back(gap);
            }
            traverseDendritesForStorage(onwardDendrite->getDendriteBranches());
        }
    }
}

std::vector<std::shared_ptr<SynapticGap>> Neuron::getSynapticGapsAxon() const {
    return synapticGapsAxon;
}

std::vector<std::shared_ptr<DendriteBouton>> Neuron::getDendriteBoutons() const {
    return dendriteBoutons;
}

std::shared_ptr<Position> Neuron::getPosition() const {
    return position;
}

void Neuron::updatePosition(std::shared_ptr<Position> &newPosition) {
    position = newPosition;
}

void Neuron::setPropagationRate(double rate) {
    propagationRate = rate;
}

double Neuron::getPropagationRate() const {
    return propagationRate;
}

void Neuron::setNeuronId(int id) {
    neuronId = id;
}

int Neuron::getNeuronId() const {
    return neuronId;
}

void Neuron::setNeuronType(int type) {
    neuronType = type;
}

int Neuron::getNeuronType() const {
    return neuronType;
}
