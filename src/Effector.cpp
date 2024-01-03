#include "../include/Effector.h"

Effector::Effector(const PositionPtr& position) : BodyComponent(position), BodyShapeComponent() {
    // On construction set a default propagation rate
    propagationRate = 0.5;
    instanceInitialised = false;
}

void Effector::initialise() {
    if (!instanceInitialised) {
        instanceInitialised = true;
    }
}

void Effector::addSynapticGap(std::shared_ptr<SynapticGap> synapticGap) {
    synapticGaps.emplace_back(std::move(synapticGap));
}

void Effector::updatePosition(const PositionPtr& newPosition) {
    position = newPosition;
}

std::vector<std::shared_ptr<SynapticGap>> Effector::getSynapticGaps() const {
    return synapticGaps;
}

double Effector::getPropagationRate() {
    // Implement the calculation based on the dendrite bouton properties
    return propagationRate;
}
