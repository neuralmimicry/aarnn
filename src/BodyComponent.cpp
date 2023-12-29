
#include "../include/BodyComponent.h"

template <typename PositionType>
BodyComponent<PositionType>::BodyComponent(PositionPtr position) : position(std::move(position)) {}

template <typename PositionType>
const typename BodyComponent<PositionType>::PositionPtr& BodyComponent<PositionType>::getPosition() const {
    return position;
}

template <typename PositionType>
void BodyComponent<PositionType>::receiveStimulation(int8_t stimulation) {
    // Implement the stimulation logic
    propagationRate += double((propagationRate * 0.01) * stimulation);
    // Clamp propagationRate within the range 0.1 to 0.9
    propagationRate = propagationRate < 0.1 || propagationRate > 0.9 ? 0 : propagationRate;
}

template <typename PositionType>
double BodyComponent<PositionType>::getPropagationRate() {
    // Implement the calculation based on the synapse properties
    return propagationRate;
}