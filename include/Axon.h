#ifndef AXON_H_INCLUDED
#define AXON_H_INCLUDED

#include "AxonBouton.h"
#include "AxonBranch.h"
#include "AxonHillock.h"
#include "BodyComponent.h"
#include "BodyShapeComponent.h"
#include "Position.h"

#include <memory>
#include <vector>

class AxonBouton;
class AxonHillock;
class AxonBranch;

class Axon
: public std::enable_shared_from_this<Axon>
, public BodyComponent<Position>
, public BodyShapeComponent
{
    public:
    explicit Axon(const PositionPtr& position, double propRate = BodyComponent<Position>::defaultPropagationRate)
    : BodyShapeComponent()
    , BodyComponent(position, propRate)
    {
    }

    ~Axon() override = default;
    void                                                          initialise();
    void                                                          updatePosition(const PositionPtr& newPosition);
    void                                                          addBranch(std::shared_ptr<AxonBranch> branch);
    [[nodiscard]] const std::vector<std::shared_ptr<AxonBranch>>& getAxonBranches() const;
    [[nodiscard]] std::shared_ptr<AxonBouton>                     getAxonBouton() const;
    double                                                        calcPropagationTime();
    void updateFromAxonHillock(std::shared_ptr<AxonHillock> parentAxonHillockPointer);
    [[nodiscard]] std::shared_ptr<AxonHillock> getParentAxonHillock() const;
    void                                      updateFromAxonBranch(std::shared_ptr<AxonBranch> parentAxonBranchPointer);
    [[nodiscard]] std::shared_ptr<AxonBranch> getParentAxonBranch() const;
    [[nodiscard]] const PositionPtr&          getPosition() const
    {
        return position;
    }

    private:
    bool                                     instanceInitialised = false;
    std::vector<std::shared_ptr<AxonBranch>> axonBranches;
    std::shared_ptr<AxonBouton>              onwardAxonBouton;
    std::shared_ptr<AxonHillock>             parentAxonHillock;
    std::shared_ptr<AxonBranch>              parentAxonBranch;
};

#endif  // AXON_H_INCLUDED
