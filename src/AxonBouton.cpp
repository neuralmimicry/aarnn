#include "AxonBouton.h"

void AxonBouton::initialise() {
    if (!instanceInitialised) {
        if (!onwardSynapticGap) {
            onwardSynapticGap = std::make_shared<SynapticGap>(std::make_shared<Position>((position->x) + 1, (position->y) + 1, (position->z) + 1));
        }
        onwardSynapticGap->initialise();
        onwardSynapticGap->updateFromAxonBouton(shared_from_this());
        instanceInitialised = true;
    }
}

void AxonBouton::updatePosition(const PositionPtr& newPosition) {
    position = newPosition;
}

void AxonBouton::addSynapticGap(const std::shared_ptr<SynapticGap>& gap) {
    gap->updateFromAxonBouton(shared_from_this());
}

void AxonBouton::connectSynapticGap(std::shared_ptr<SynapticGap> gap) {
    // Implement the connection logic
}

std::shared_ptr<SynapticGap> AxonBouton::getSynapticGap() const {
    return onwardSynapticGap;
}

void AxonBouton::setNeuron(std::weak_ptr<Neuron> parentNeuron) {
    this->neuron = std::move(parentNeuron);
}

void AxonBouton::updateFromAxon(std::shared_ptr<Axon> parentAxonPointer) {
    parentAxon = std::move(parentAxonPointer);
}

std::shared_ptr<Axon> AxonBouton::getParentAxon() const {
    return parentAxon;
}