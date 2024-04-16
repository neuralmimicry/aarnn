#ifndef AXONBRANCH_H_INCLUDED
#define AXONBRANCH_H_INCLUDED

#include "Axon.h"
#include "BodyComponent.h"
#include "BodyShapeComponent.h"
#include "Position.h"

#include <memory>
#include <vector>

class Axon;

class AxonBranch
: public std::enable_shared_from_this<AxonBranch>
, public BodyComponent<Position>
, public BodyShapeComponent
{
    public:
    explicit AxonBranch(const PositionPtr &position, double propRate = BodyComponent<Position>::defaultPropagationRate)
    : BodyShapeComponent()
    , BodyComponent(position, propRate)
    {
    }
    ~AxonBranch() override = default;
    
    void initialise();
    void updatePosition(const PositionPtr &newPosition);
    void connectAxon(std::shared_ptr<Axon> axon);
    void updateFromAxon(std::shared_ptr<Axon> parentPointer);

    [[nodiscard]] const std::vector<std::shared_ptr<Axon>> &getAxons() const
    {
        return onwardAxons;
    }

    [[nodiscard]] std::shared_ptr<Axon> getParentAxon() const
    {
        return parentAxon;
    }

    [[nodiscard]] const PositionPtr &getPosition() const
    {
        return position;
    }

    private:
    bool                               instanceInitialised = false;
    std::vector<std::shared_ptr<Axon>> onwardAxons;
    std::shared_ptr<Axon>              parentAxon;
};

#endif  // AXONBRANCH_H_INCLUDED