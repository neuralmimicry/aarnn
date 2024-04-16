#ifndef AXONHILLOCK_H_INCLUDED
#define AXONHILLOCK_H_INCLUDED

#include "Axon.h"
#include "BodyComponent.h"
#include "BodyShapeComponent.h"
#include "Position.h"
#include "Soma.h"

#include <memory>
#include <vector>

class Soma;
class Axon;

class AxonHillock
: public std::enable_shared_from_this<AxonHillock>
, public BodyComponent<Position>
, public BodyShapeComponent
{
    public:
    explicit AxonHillock(const PositionPtr &position, double propRate = BodyComponent<Position>::defaultPropagationRate)
    : BodyShapeComponent()
    , BodyComponent(position, propRate)
    {
    }

    ~AxonHillock() override = default;

    void initialise();
    void updatePosition(const PositionPtr &newPosition);
    void updateFromSoma(std::shared_ptr<Soma> parentPointer);

    [[nodiscard]] std::shared_ptr<Axon> getAxon() const
    {
        return onwardAxon;
    }

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

#endif  // AXONHILLOCK_H_INCLUDED