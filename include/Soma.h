#ifndef SOMA_H
#define SOMA_H

#include <memory>
#include <vector>
#include "AxonHillock.h"
#include "DendriteBranch.h"
#include "NeuronalComponent.h"
#include "Position.h"

// Forward declarations
class AxonHillock;
class SynapticGap;
class DendriteBranch;
class Neuron;

class Soma : public NeuronalComponent
{
public:
    explicit Soma(const std::shared_ptr<Position>& position);

    ~Soma() override = default;

    // Override methods from NeuronalComponent
    void initialise() override;

    // Soma-specific methods
    [[nodiscard]] std::shared_ptr<AxonHillock> getAxonHillock() const;
    void addDendriteBranch(std::shared_ptr<DendriteBranch> dendriteBranch);
    [[nodiscard]] const std::vector<std::shared_ptr<DendriteBranch>>& getDendriteBranches() const;
    void updateFromNeuron(std::shared_ptr<Neuron> parentPointer);
    [[nodiscard]] std::shared_ptr<Neuron> getParentNeuron() const;
    void update(double deltaTime);
    void setSomaId(int id);
    int getSomaId() const;

private:
    // Member variables
    std::vector<std::shared_ptr<SynapticGap>> synapticGaps;
    std::vector<std::shared_ptr<DendriteBranch>> dendriteBranches;
    std::shared_ptr<AxonHillock> onwardAxonHillock;
    std::shared_ptr<Neuron> parentNeuron;
    int somaId;
};

#endif // SOMA_H
