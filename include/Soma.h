#ifndef SOMA_H
#define SOMA_H

#include <memory>

#include "BodyComponent.h"
#include "BodyShapeComponent.h"
#include "AxonHillock.h"
#include "Position.h"
#include "DendriteBranch.h"
#include "Neuron.h"
#include "SynapticGap.h"

class AxonHillock;
class SynapticGap;
class DendriteBranch;
class Neuron;

class Soma : public std::enable_shared_from_this<Soma>, public BodyComponent<Position>, public BodyShapeComponent
{
public:
    explicit Soma(const PositionPtr &position, double propRate = 0.5)
    : BodyComponent(position)
    , BodyShapeComponent()
    {
        propagationRate = propRate;
    }

    ~Soma() override = default;
    void initialise();
    void updatePosition(const PositionPtr &newPosition);
    [[nodiscard]] std::shared_ptr<AxonHillock> getAxonHillock() const { return onwardAxonHillock; }
    void addDendriteBranch(std::shared_ptr<DendriteBranch> dendriteBranch);
    [[nodiscard]] const std::vector<std::shared_ptr<DendriteBranch>> &getDendriteBranches() const { return dendriteBranches; }
    void updateFromNeuron(std::shared_ptr<Neuron> parentPointer);
    [[nodiscard]] std::shared_ptr<Neuron> getParentNeuron() const { return parentNeuron; }
    [[nodiscard]] const PositionPtr &getPosition() const { return position; }

private:
    bool instanceInitialised = false;
    std::vector<std::shared_ptr<SynapticGap>> synapticGaps;
    std::vector<std::shared_ptr<DendriteBranch>> dendriteBranches;
    std::shared_ptr<AxonHillock> onwardAxonHillock;
    std::shared_ptr<Neuron> parentNeuron;
};

#endif