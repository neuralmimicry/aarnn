#ifndef DENDRITEBRANCH_H
#define DENDRITEBRANCH_H

#include "BodyComponent.h"
#include "BodyShapeComponent.h"
#include "Dendrite.h"
#include "Position.h"
#include "Soma.h"
#include "SynapticGap.h"

#include <memory>

class Soma;
class Dendrite;

class DendriteBranch
: public std::enable_shared_from_this<DendriteBranch>
, public BodyComponent<Position>
, public BodyShapeComponent
{
    public:
    explicit DendriteBranch(const PositionPtr &position,
                            double             propRate = BodyComponent<Position>::defaultPropagationRate)
    : BodyComponent<Position>(position, propRate)
    , BodyShapeComponent()
    {
    }

    ~DendriteBranch() override = default;
    void                                                 initialise();
    void                                                 updatePosition(const PositionPtr &newPosition);
    void                                                 connectDendrite(std::shared_ptr<Dendrite> dendrite);
    [[nodiscard]] std::vector<std::shared_ptr<Dendrite>> getDendrites() const
    {
        return onwardDendrites;
    }
    void                                updateFromSoma(std::shared_ptr<Soma> parentSomaPointer);
    [[nodiscard]] std::shared_ptr<Soma> getParentSoma() const
    {
        return parentSoma;
    }
    void                                    updateFromDendrite(std::shared_ptr<Dendrite> parentDendritePointer);
    [[nodiscard]] std::shared_ptr<Dendrite> getParentDendrite() const
    {
        return parentDendrite;
    }
    [[nodiscard]] const PositionPtr &getPosition() const
    {
        return position;
    }

    private:
    bool                                   instanceInitialised = false;  // Initially, the Dendrite is not initialised
    std::vector<std::shared_ptr<Dendrite>> onwardDendrites;
    std::shared_ptr<Soma>                  parentSoma;
    std::shared_ptr<Dendrite>              parentDendrite;
};

#endif