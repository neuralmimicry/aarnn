#ifndef SYNAPTICGAP_H
#define SYNAPTICGAP_H

#include <cmath>
#include <memory>
#include <vector>
#include "NeuronalComponent.h"
#include "Position.h"
#include "SensoryReceptor.h"
#include "Effector.h"

// Forward declarations
class Effector;
class DendriteBouton;
class AxonBouton;
class SensoryReceptor;
class AxonHillock;

class SynapticGap : public NeuronalComponent
{
public:
    explicit SynapticGap(const std::shared_ptr<Position>& position);

    ~SynapticGap() override = default;

    // Override methods from NeuronalComponent
    void initialise() override;
    void update(double deltaTime);

    // SynapticGap-specific methods
    // Method to check if the SynapticGap has been associated
    bool isAssociated() const;
    // Method to set the SynapticGap as associated
    void setAsAssociated();

    void updateFromSensoryReceptor(std::shared_ptr<SensoryReceptor> parentSensoryReceptorPointer);
    void updateFromDendriteBouton(std::shared_ptr<DendriteBouton> parentDendriteBoutonPointer);
    void updateFromAxonBouton(std::shared_ptr<AxonBouton> parentAxonBoutonPointer);
    void updateFromEffector(std::shared_ptr<Effector> parentEffectorPointer);

    [[nodiscard]] std::shared_ptr<SensoryReceptor> getParentSensoryReceptor() const;
    [[nodiscard]] std::shared_ptr<Effector> getParentEffector() const;
    [[nodiscard]] std::shared_ptr<AxonBouton> getParentAxonBouton() const;
    [[nodiscard]] std::shared_ptr<DendriteBouton> getParentDendriteBouton() const;

    // Energy and signal propagation methods
    void updateComponent(double time, double energy);
    double calculateEnergy(double currentTime, double currentEnergyLevel);
    double calculateWaveform(double currentTime) const;
    double propagationTime();
    void setSynapticGapId(int id);
    int getSynapticGapId() const;

private:
    // Member variables
    bool associated = false;  // Initially, the SynapticGap is not associated
    std::shared_ptr<Effector> parentEffector;
    std::shared_ptr<SensoryReceptor> parentSensoryReceptor;
    std::shared_ptr<AxonBouton> parentAxonBouton;
    std::shared_ptr<DendriteBouton> parentDendriteBouton;

    // Energy and signal properties
    double attack = 0.1;
    double decay = 0.2;
    double sustain = 0.7;
    double release = 0.3;
    double frequencyResponse = 1.0;
    double phaseShift = 0.0;
    double previousTime = 0.0;
    double componentEnergyLevel = 0.0;
    double minPropagationTime = 0.1;
    double maxPropagationTime = 1.0;
    double lastCallTime = 0.0;
    int callCount = 0;
    int synapticGapId;
};

#endif  // SYNAPTICGAP_H
