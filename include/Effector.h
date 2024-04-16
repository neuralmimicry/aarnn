#ifndef EFFECTOR_H_INCLUDED
#define EFFECTOR_H_INCLUDED

#include "BodyComponent.h"  // Include necessary headers
#include "BodyShapeComponent.h"
#include "Position.h"
#include "SynapticGap.h"

#include <memory>
#include <vector>

class SynapticGap;
class Position;

class Effector
: public std::enable_shared_from_this<Effector>
, public BodyComponent<Position>
, public BodyShapeComponent
{
    public:
    explicit Effector(const PositionPtr& position, double propRate = BodyComponent<Position>::defaultPropagationRate)
    : BodyShapeComponent()
    , BodyComponent(position, propRate)
    {
        instanceInitialised = false;
    }

    ~Effector() override = default;
    void   initialise();
    void   addSynapticGap(std::shared_ptr<SynapticGap> synapticGap);
    void   updatePosition(const PositionPtr& newPosition);
    double getPropagationRate() override;

    [[nodiscard]] std::vector<std::shared_ptr<SynapticGap>> getSynapticGaps() const;

    [[nodiscard]] const PositionPtr& getPosition() const
    {
        return position;
    }

    private:
    bool                                      instanceInitialised;
    std::vector<std::shared_ptr<SynapticGap>> synapticGaps;
};

#endif  // EFFECTOR_H_INCLUDED
