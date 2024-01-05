#ifndef DENDRITEBOUTON_H
#define DENDRITEBOUTON_H


#include <memory>
#include "BodyComponent.h"
#include "BodyShapeComponent.h"
#include "Position.h"
#include "SynapticGap.h"
#include "Neuron.h"
#include "Dendrite.h"

class Dendrite;
class DendriteBranch;
class SynapticGap;
class Neuron;

class DendriteBouton : public std::enable_shared_from_this<DendriteBouton>, public BodyComponent<Position>, public BodyShapeComponent {
public:
    explicit DendriteBouton(const PositionPtr &position, double propRate = 0.5)
    : BodyComponent(position)
    , BodyShapeComponent()
    {
        propagationRate = propRate;
    }
    ~DendriteBouton() override = default;
    void initialise() ;
    void connectSynapticGap(std::shared_ptr<SynapticGap> gap);
    void updatePosition(const PositionPtr& newPosition) ;
    [[nodiscard]] std::shared_ptr<SynapticGap> getSynapticGap() const { return onwardSynapticGap; }
    double getPropagationRate() override {return propagationRate; }
    void setNeuron(std::weak_ptr<Neuron> parentNeuron);
    void updateFromDendrite(std::shared_ptr<Dendrite> parentDendritePointer) { parentDendrite = std::move(parentDendritePointer); }
    [[nodiscard]] std::shared_ptr<Dendrite> getParentDendrite() const { return parentDendrite; }
    [[nodiscard]] const PositionPtr& getPosition() const { return position; }
private:
    bool instanceInitialised = false;  // Initially, the DendriteBouton is not initialised
    std::shared_ptr<SynapticGap> onwardSynapticGap;
    std::weak_ptr<Neuron> neuron;
    std::shared_ptr<Dendrite> parentDendrite;
};

#endif