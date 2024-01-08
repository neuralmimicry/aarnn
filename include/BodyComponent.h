// BodyComponent.h

#ifndef BODY_COMPONENT_H
#define BODY_COMPONENT_H

#include <memory>

template <typename PositionType>
class BodyComponent {
public:
    using PositionPtr = std::shared_ptr<PositionType>;

    static constexpr double defaultPropagationRate = 0.5;

    explicit BodyComponent(PositionPtr  position, double proRate) : 
        position(std::move(position)) 
    { 
            propagationRate = proRate;
    }

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
    double propagationRate{defaultPropagationRate};
};

#endif // BODY_COMPONENT_H
