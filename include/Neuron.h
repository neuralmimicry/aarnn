#ifndef NEURON_H
#define NEURON_H

#include <iostream>
#include <memory>
#include <iostream>
#include <vector>

#include "Soma.h"
#include "Position.h"
#include "BodyShapeComponent.h"
#include "BodyComponent.h"
#include "Axon.h"
#include "SynapticGap.h"
#include "AxonBouton.h"
#include "DendriteBouton.h"
#include "ThreadSafeQueue.h"
#include "DendriteBranch.h"

class AxonBouton;
class Axon;
class SynapticGap;
class DendriteBouton;

class Neuron : public std::enable_shared_from_this<Neuron>, public BodyComponent<Position>, public BodyShapeComponent {
public:
    Neuron(const PositionPtr& position);
    ~Neuron() override;
    void initialise();
    std::shared_ptr<Soma> getSoma();
    void updatePosition(const PositionPtr& newPosition);
    void storeAllSynapticGapsAxon();
    void storeAllSynapticGapsDendrite();
    void addSynapticGapAxon(std::shared_ptr<SynapticGap> synapticGap);
    std::vector<std::shared_ptr<SynapticGap>> synapticGapsAxon;
    std::vector<std::shared_ptr<SynapticGap>> synapticGapsDendrite;
    std::vector<std::shared_ptr<DendriteBouton>> dendriteBoutons;
private:
    bool instanceInitialised = false;
    std::shared_ptr<Soma> soma;
    //std::vector<std::shared_ptr<SynapticGap>> synapticGapsAxon;
    //std::vector<std::shared_ptr<SynapticGap>> synapticGapsDendrite;
    std::vector<std::shared_ptr<AxonBouton>> axonBoutons;
    //std::vector<std::shared_ptr<DendriteBouton>> dendriteBoutons;
    std::shared_ptr<SynapticGap> traverseAxons(const std::shared_ptr<Axon>& axon, const PositionPtr& positionPtr);
    std::shared_ptr<SynapticGap> traverseDendrites(const std::shared_ptr<Dendrite>& dendrite, const PositionPtr& positionPtr);
    void traverseAxonsForStorage(const std::shared_ptr<Axon>& axon);
    void traverseDendritesForStorage(const std::vector<std::shared_ptr<DendriteBranch>>& dendriteBranches);
};

#endif // NEURON_H
