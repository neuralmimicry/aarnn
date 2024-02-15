#ifndef DENDRITE_H
#define DENDRITE_H

#include "BodyShapeComponent.h"
#include "DendriteBouton.h"
#include "DendriteBranch.h"
#include "Position.h"

#include <iostream>

class DendriteBouton;
class DendriteBranch;

class Dendrite
: public std::enable_shared_from_this<Dendrite>
, public BodyComponent<Position>
, public BodyShapeComponent
{
    private:
    bool instanceInitialised = false;  // Initially, the Dendrite is not initialised
    std::vector<std::shared_ptr<DendriteBranch>> dendriteBranches;
    std::shared_ptr<DendriteBranch>              dendriteBranch;
    std::shared_ptr<DendriteBouton>              dendriteBouton;
    std::shared_ptr<DendriteBranch>              parentDendriteBranch;

    public:
    explicit Dendrite(const PositionPtr &position, double propRate = BodyComponent<Position>::defaultPropagationRate)
    : BodyComponent<Position>(position, propRate)
    , BodyShapeComponent()
    {
    }

    ~Dendrite() override = default;
    void initialise();
    void updatePosition(const PositionPtr &newPosition)
    {
        position = newPosition;
    }
    void                                                              addBranch(std::shared_ptr<DendriteBranch> branch);
    [[nodiscard]] const std::vector<std::shared_ptr<DendriteBranch>> &getDendriteBranches() const
    {
        return dendriteBranches;
    }
    [[nodiscard]] const std::shared_ptr<DendriteBouton> &getDendriteBouton() const
    {
        return dendriteBouton;
    }
    void updateFromDendriteBranch(std::shared_ptr<DendriteBranch> parentDendriteBranchPointer)
    {
        parentDendriteBranch = std::move(parentDendriteBranchPointer);
    }
    [[nodiscard]] std::shared_ptr<DendriteBranch> getParentDendriteBranch() const
    {
        return parentDendriteBranch;
    }
    [[nodiscard]] const PositionPtr &getPosition() const
    {
        return position;
    }
};

#endif  // AARNN_DENDRITE_H
