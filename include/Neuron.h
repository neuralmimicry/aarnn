#ifndef NEURON_H
#define NEURON_H

#include <memory>
#include <vector>
#include "Position.h"
#include "utils.h"

class AxonBouton;
class Soma;
class SynapticGap;
class DendriteBouton;
class DendriteBranch;
class Dendrite;
class Axon;

class Neuron : public std::enable_shared_from_this<Neuron> {
public:
    explicit Neuron(const std::shared_ptr<Position> &position)
            : instanceInitialised(false), neuronId(nextNeuronId++) {}

    std::shared_ptr<Soma> getSoma();
    void initialise();
    void addSynapticGapDendrite(std::shared_ptr<SynapticGap> synapticGap);
    void updatePosition(std::shared_ptr<Position> &newPosition);
    void storeAllSynapticGapsAxon();
    void storeAllSynapticGapsDendrite();
    void addSynapticGapAxon(std::shared_ptr<SynapticGap> synapticGap);

    std::vector<std::shared_ptr<SynapticGap>> getSynapticGapsAxon() const;
    std::vector<std::shared_ptr<DendriteBouton>> getDendriteBoutons() const;

    std::shared_ptr<Position> getPosition() const;
    int getNeuronId() const { return neuronId; }

private:
    void traverseAxonsForStorage(const std::shared_ptr<Axon> &axon);
    void traverseDendritesForStorage(const std::vector<std::shared_ptr<DendriteBranch>> &dendriteBranches);
    std::shared_ptr<SynapticGap> traverseAxons(const std::shared_ptr<Axon> &axon, const std::shared_ptr<Position> &positionPtr);
    std::shared_ptr<SynapticGap> traverseDendrites(const std::shared_ptr<Dendrite> &dendrite, const std::shared_ptr<Position> &positionPtr);

    static int nextNeuronId;
    int neuronId;
    std::shared_ptr<Position> position;
    std::shared_ptr<Soma> soma;
    bool instanceInitialised;
    std::vector<std::shared_ptr<SynapticGap>> synapticGapsAxon;
    std::vector<std::shared_ptr<SynapticGap>> synapticGapsDendrite;
    std::vector<std::shared_ptr<DendriteBouton>> dendriteBoutons;
    std::vector<std::shared_ptr<AxonBouton>> axonBoutons;
};

#endif // NEURON_H
