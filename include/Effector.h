#ifndef EFFECTOR_H
#define EFFECTOR_H

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
    explicit Effector(const PositionPtr &position, double propRate = BodyComponent<Position>::defaultPropagationRate)
    : BodyComponent<Position>(position, propRate)
    , BodyShapeComponent()
    {
        instanceInitialised = false;
    }

    ~Effector() override = default;
    void                                                    initialise();
    void                                                    addSynapticGap(std::shared_ptr<SynapticGap> synapticGap);
    void                                                    updatePosition(const PositionPtr &newPosition);
    [[nodiscard]] std::vector<std::shared_ptr<SynapticGap>> getSynapticGaps() const;
    double                                                  getPropagationRate() override;
    [[nodiscard]] const PositionPtr                        &getPosition() const
    {
        return position;
    }

    private:
    bool                                      instanceInitialised;
    std::vector<std::shared_ptr<SynapticGap>> synapticGaps;
};

#endif  // EFFECTOR_H
