// BodyComponent.h

#ifndef BODY_COMPONENT_H
#define BODY_COMPONENT_H

#include <memory>

template <typename PositionType>
class BodyComponent {
public:
    using PositionPtr = std::shared_ptr<PositionType>;
    explicit BodyComponent(PositionPtr position);
    virtual ~BodyComponent() = default;
    [[nodiscard]] virtual const PositionPtr& getPosition() const;
    virtual void receiveStimulation(int8_t stimulation);
    virtual double getPropagationRate();

protected:
    PositionPtr position;
    double propagationRate{0.5};
};

#endif // BODY_COMPONENT_H
