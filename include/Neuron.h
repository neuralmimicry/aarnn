#ifndef NEURON_H
#define NEURON_H

#include <memory>
#include <vector>
#include "NeuronalComponent.h"
#include "Position.h"
#include "utils.h"
#include "Cluster.h"

// Forward declarations to avoid circular dependencies
class Cluster;
class AxonBouton;
class Soma;
class SynapticGap;
class DendriteBouton;
class DendriteBranch;
class Dendrite;
class Axon;

class Neuron : public NeuronalComponent
{
public:
    explicit Neuron(const std::shared_ptr<Position>& position);

    // Methods
    std::shared_ptr<Soma> getSoma();
    void initialise() override;
    void addSynapticGapDendrite(std::shared_ptr<SynapticGap> synapticGap);
    void storeAllSynapticGapsAxon();
    void storeAllSynapticGapsDendrite();
    void addSynapticGapAxon(std::shared_ptr<SynapticGap> synapticGap);
    std::vector<std::shared_ptr<SynapticGap>> getSynapticGapsAxon() const;
    std::vector<std::shared_ptr<DendriteBouton>> getDendriteBoutons() const;
    int getNeuronId() const;
    void setNeuronId(int id);
    void setPropagationRate(double rate);
    double getPropagationRate() const;
    void setNeuronType(int type);
    int getNeuronType() const;
    void update(double deltaTime);
    void updateFromCluster(std::shared_ptr<Cluster> parentPointer);
    std::shared_ptr<Cluster> getParentCluster() const;

private:
    void traverseAxonsForStorage(const std::shared_ptr<Axon>& axon);
    void traverseDendritesForStorage(const std::vector<std::shared_ptr<DendriteBranch>>& dendriteBranches);
    std::shared_ptr<SynapticGap> traverseAxons(const std::shared_ptr<Axon>& axon, const std::shared_ptr<Position>& positionPtr);
    std::shared_ptr<SynapticGap> traverseDendrites(const std::shared_ptr<Dendrite>& dendrite, const std::shared_ptr<Position>& positionPtr);

    // Static member for generating unique neuron IDs
    static int nextNeuronId;

    // Member variables
    int neuronId;
    int neuronType;
    std::shared_ptr<Soma> soma;
    std::vector<std::shared_ptr<SynapticGap>> synapticGapsAxon;
    std::vector<std::shared_ptr<SynapticGap>> synapticGapsDendrite;
    std::vector<std::shared_ptr<DendriteBouton>> dendriteBoutons;
    std::vector<std::shared_ptr<AxonBouton>> axonBoutons;
    double propagationRate;
    std::shared_ptr<Cluster> parentCluster;
};

#endif // NEURON_H
