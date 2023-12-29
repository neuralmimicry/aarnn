// Axon.cpp

#include "../include/Axon.h"

Axon::Axon(const PositionPtr& position) : BodyComponent(position), BodyShapeComponent() {
    // On construction set a default propagation rate
    propagationRate = 0.5;
}

void Axon::initialise() {
    if (!instanceInitialised) {
        if (!onwardAxonBouton) {
            onwardAxonBouton = std::make_shared<AxonBouton>(std::make_shared<Position>((position->x) + 1, (position->y) + 1, (position->z) + 1));
        }
        onwardAxonBouton->initialise();
        onwardAxonBouton->updateFromAxon(shared_from_this());
        instanceInitialised = true;
    }
}

void Axon::updatePosition(const PositionPtr& newPosition) {
    position = newPosition;
}

void Axon::addBranch(std::shared_ptr<AxonBranch> branch) {
    axonBranches.push_back(branch);
}

const std::vector<std::shared_ptr<AxonBranch>>& Axon::getAxonBranches() const {
    return axonBranches;
}

std::shared_ptr<AxonBouton> Axon::getAxonBouton() const {
    return onwardAxonBouton;
}

double Axon::calcPropagationTime() {
    // Implement the calculation
    return 0.0;
}

void Axon::updateFromAxonHillock(std::shared_ptr<AxonHillock> parentAxonHillockPointer) {
    parentAxonHillock = std::move(parentAxonHillockPointer);
}

std::shared_ptr<AxonHillock> Axon::getParentAxonHillock() const {
    return parentAxonHillock;
}

void Axon::updateFromAxonBranch(std::shared_ptr<AxonBranch> parentAxonBranchPointer) {
    parentAxonBranch = std::move(parentAxonBranchPointer);
}

std::shared_ptr<AxonBranch> Axon::getParentAxonBranch() const {
    return parentAxonBranch;
}

const PositionPtr& Axon::getPosition() const {
    return position;
}
