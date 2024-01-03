// BodyComponent.h

#ifndef BODY_COMPONENT_H
#define BODY_COMPONENT_H

#include <memory>

template <typename PositionType>
class BodyComponent {
public:
    using PositionPtr = std::shared_ptr<PositionType>;

    explicit BodyComponent(PositionPtr  position) : position(std::move(position)) {}
    virtual ~BodyComponent() = default;

    [[nodiscard]] virtual const PositionPtr& getPosition() const {
        return position;
    }

    virtual void receiveStimulation(int8_t stimulation) {
        // Implement the stimulation logic
        propagationRate += double((propagationRate * 0.01) * stimulation);
        // Clamp propagationRate within the range 0.1 to 0.9
        propagationRate = propagationRate < 0.1 || propagationRate > 0.9 ? 0 : propagationRate;
    }

    virtual double getPropagationRate() {
        // Implement the calculation based on the synapse properties
        return propagationRate;
    }

protected:
    PositionPtr position;
    double propagationRate{0.5};
};

#endif // BODY_COMPONENT_H
