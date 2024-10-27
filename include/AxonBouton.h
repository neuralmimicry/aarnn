#ifndef AXONBOUTON_H
#define AXONBOUTON_H

#include <memory>
#include "NeuronalComponent.h"
#include "Position.h"
#include "SynapticGap.h"

// Forward declarations
class Axon;
class Neuron;

class AxonBouton : public NeuronalComponent
{
public:
    explicit AxonBouton(const std::shared_ptr<Position>& position);

    ~AxonBouton() override = default;

    // Override methods from NeuronalComponent
    void initialise() override;
    void update(double deltaTime);

    // AxonBouton-specific methods
    void addSynapticGap(const std::shared_ptr<SynapticGap>& gap);
    void connectSynapticGap(std::shared_ptr<SynapticGap> gap);
    [[nodiscard]] std::shared_ptr<SynapticGap> getSynapticGap() const;
    void setNeuron(std::weak_ptr<Neuron> parentNeuron);
    void updateFromAxon(std::shared_ptr<Axon> parentAxonPointer);
    [[nodiscard]] std::shared_ptr<Axon> getParentAxon() const;
    void setAxonBoutonId(int id);
    int getAxonBoutonId() const;

private:
    // Member variables
    std::shared_ptr<SynapticGap> onwardSynapticGap;
    std::weak_ptr<Neuron> neuron;
    std::shared_ptr<Axon> parentAxon;
    int axonBoutonId;
};

#endif  // AXONBOUTON_H
