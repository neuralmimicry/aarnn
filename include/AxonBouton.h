#ifndef AXONBOUTON_H
#define AXONBOUTON_H

#include "Axon.h"
#include "BodyComponent.h"
#include "BodyShapeComponent.h"
#include "Neuron.h"
#include "Position.h"
#include "SynapticGap.h"

#include <memory>

class Axon;
class SynapticGap;
class Position;

class Neuron;

class AxonBouton
: public std::enable_shared_from_this<AxonBouton>
, public BodyComponent<Position>
, public BodyShapeComponent
{
    public:
    explicit AxonBouton(const PositionPtr &position, double propRate = BodyComponent<Position>::defaultPropagationRate)
    : BodyComponent<Position>(position, propRate)
    , BodyShapeComponent()
    {
    }
    ~AxonBouton() override = default;
    void                                       initialise();
    void                                       updatePosition(const PositionPtr &newPosition);
    void                                       addSynapticGap(const std::shared_ptr<SynapticGap> &gap);
    void                                       connectSynapticGap(std::shared_ptr<SynapticGap> gap);
    [[nodiscard]] std::shared_ptr<SynapticGap> getSynapticGap() const;
    void                                       setNeuron(std::weak_ptr<Neuron> parentNeuron);
    void                                       updateFromAxon(std::shared_ptr<Axon> parentAxonPointer);
    [[nodiscard]] std::shared_ptr<Axon>        getParentAxon() const;
    [[nodiscard]] const PositionPtr           &getPosition() const
    {
        return position;
    }

    private:
    bool                         instanceInitialised = false;
    std::shared_ptr<SynapticGap> onwardSynapticGap;
    std::weak_ptr<Neuron>        neuron;
    std::shared_ptr<Axon>        parentAxon;
};

#endif  // AXON_BOUTON_H
