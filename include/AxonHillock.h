#ifndef AXONHILLOCK_H
#define AXONHILLOCK_H

#include "BodyComponent.h"
#include "BodyShapeComponent.h"
#include "Position.h"
#include <memory>
#include <vector>
#include "utils.h"

class Soma;
class Axon;

class AxonHillock
: public std::enable_shared_from_this<AxonHillock>
, public BodyComponent<Position>
, public BodyShapeComponent
{
    public:
    explicit AxonHillock(const PositionPtr &position, double propRate = BodyComponent<Position>::defaultPropagationRate)
    : BodyComponent<Position>(position, propRate)
    , BodyShapeComponent()
    {
    }

    ~AxonHillock() override = default;
    void                                initialise();
    void                                updatePosition(const PositionPtr &newPosition);
    [[nodiscard]] std::shared_ptr<Axon> getAxon() const
    {
        return onwardAxon;
    }
    void                                updateFromSoma(std::shared_ptr<Soma> parentPointer);
    [[nodiscard]] std::shared_ptr<Soma> getParentSoma() const
    {
        return parentSoma;
    }
    [[nodiscard]] const PositionPtr &getPosition() const
    {
        return position;
    }

    private:
    bool                  instanceInitialised = false;
    std::shared_ptr<Axon> onwardAxon;
    std::shared_ptr<Soma> parentSoma;
};

#endif