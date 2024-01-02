#ifndef SYNAPTICGAP_H
#define SYNAPTICGAP_H

#include <memory>
#include <vector>
#include "math.h"

#include "BodyComponent.h"
#include "BodyShapeComponent.h"
#include "AxonHillock.h"
#include "Position.h"
#include "DendriteBranch.h"
#include "Neuron.h"
#include "AxonHillock.h"
#include "SensoryReceptor.h"
#include "Effector.h"

class Effector;
class DendriteBouton;
class AxonBouton;
class SensoryReceptor;

class SynapticGap : public std::enable_shared_from_this<SynapticGap>, public BodyComponent<Position>, public BodyShapeComponent
{
public:
    explicit SynapticGap(const PositionPtr &position) : BodyComponent(position), BodyShapeComponent()
    {
        // On construction set a default propagation rate
        propagationRate = 0.5;
    }
    ~SynapticGap() override = default;
    void initialise();
    void updatePosition(const PositionPtr &newPosition);
    // Method to check if the SynapticGap has been associated
    bool isAssociated() const;
    // Method to set the SynapticGap as associated
    void setAsAssociated();
    void updateFromSensoryReceptor(std::shared_ptr<SensoryReceptor> parentSensoryReceptorPointer);
    [[nodiscard]] std::shared_ptr<SensoryReceptor> getParentSensoryReceptor() const { return parentSensoryReceptor; }
    void updateFromEffector(std::shared_ptr<Effector> &parentEffectorPointer);
    [[nodiscard]] std::shared_ptr<Effector> getParentEffector() const { return parentEffector; }
    void updateFromAxonBouton(std::shared_ptr<AxonBouton> parentAxonBoutonPointer);
    [[nodiscard]] std::shared_ptr<AxonBouton> getParentAxonBouton() const { return parentAxonBouton; }
    void updateFromDendriteBouton(std::shared_ptr<DendriteBouton> parentDendriteBoutonPointer);
    [[nodiscard]] std::shared_ptr<DendriteBouton> getParentDendriteBouton() const { return parentDendriteBouton; }
    void updateComponent(double time, double energy);
    double calculateEnergy(double currentTime, double currentEnergyLevel);
    double calculateWaveform(double currentTime) const;
    double propagationTime();
    [[nodiscard]] const PositionPtr &getPosition() const { return position; }

private:
    bool instanceInitialised = false; // Initially, the SynapticGap is not initialised
    bool associated = false;          // Initially, the SynapticGap is not associated with a DendriteBouton
    std::shared_ptr<Effector> parentEffector;
    std::shared_ptr<SensoryReceptor> parentSensoryReceptor;
    std::shared_ptr<AxonBouton> parentAxonBouton;
    std::shared_ptr<DendriteBouton> parentDendriteBouton;
    double attack;
    double decay;
    double sustain;
    double release;
    double frequencyResponse;
    double phaseShift;
    double previousTime;
    double energyLevel;
    double componentEnergyLevel;
    double minPropagationTime;
    double maxPropagationTime;
    double lastCallTime;
    int callCount;
};
#endif // SYNAPTICGAP_H