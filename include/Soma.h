#ifndef SOMA_H
#define SOMA_H

#include "AxonHillock.h"
#include "BodyComponent.h"
#include "BodyShapeComponent.h"
#include "DendriteBranch.h"
#include "Position.h"

#include <memory>
#include <vector>

class AxonHillock;
class SynapticGap;
class DendriteBranch;
class Neuron;

class Soma
: public std::enable_shared_from_this<Soma>
, public BodyComponent<Position>
, public BodyShapeComponent
{
    public:
    explicit Soma(const PositionPtr &position, double propRate = BodyComponent<Position>::defaultPropagationRate)
    : BodyComponent<Position>(position, propRate)
    , BodyShapeComponent()
    {
    }

    ~Soma() override = default;
    void                                       initialise();
    void                                       updatePosition(const PositionPtr &newPosition);
    [[nodiscard]] std::shared_ptr<AxonHillock> getAxonHillock() const
    {
        return onwardAxonHillock;
    }
    void addDendriteBranch(std::shared_ptr<DendriteBranch> dendriteBranch);
    [[nodiscard]] const std::vector<std::shared_ptr<DendriteBranch>> &getDendriteBranches() const
    {
        return dendriteBranches;
    }
    void                                  updateFromNeuron(std::shared_ptr<Neuron> parentPointer);
    [[nodiscard]] std::shared_ptr<Neuron> getParentNeuron() const
    {
        return parentNeuron;
    }

    [[nodiscard]] const PositionPtr &getPosition() const override
    {
        return position;
    }

    private:
    bool                                         instanceInitialised = false;
    std::vector<std::shared_ptr<SynapticGap>>    synapticGaps;
    std::vector<std::shared_ptr<DendriteBranch>> dendriteBranches;
    std::shared_ptr<AxonHillock>                 onwardAxonHillock;
    std::shared_ptr<Neuron>                      parentNeuron;
};

#endif