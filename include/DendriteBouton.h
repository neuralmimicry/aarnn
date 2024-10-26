#ifndef DENDRITEBOUTON_H
#define DENDRITEBOUTON_H

#include "NeuronalComponent.h"
#include "Dendrite.h"
#include "Neuron.h"
#include "Position.h"
#include "SynapticGap.h"

#include <memory>

class Dendrite;
class DendriteBranch;
class SynapticGap;
class Neuron;

class DendriteBouton : public NeuronalComponent
{
public:
    explicit DendriteBouton(const std::shared_ptr<Position>& position);

    ~DendriteBouton() override = default;

    void initialise() override;
    void connectSynapticGap(std::shared_ptr<SynapticGap> gap);
    [[nodiscard]] std::shared_ptr<SynapticGap> getSynapticGap() const;

    void setNeuron(std::weak_ptr<Neuron> parentNeuron);
    void updateFromDendrite(std::shared_ptr<Dendrite> parentDendritePointer);
    [[nodiscard]] std::shared_ptr<Dendrite> getParentDendrite() const;
    void update(double deltaTime);


private:
    bool instanceInitialised = false;  // Initially, the DendriteBouton is not initialized
    std::shared_ptr<SynapticGap> onwardSynapticGap;
    std::weak_ptr<Neuron> neuron;
    std::shared_ptr<Dendrite> parentDendrite;
};

#endif // DENDRITEBOUTON_H
