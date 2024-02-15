#ifndef SYNAPTICGAP_H
#define SYNAPTICGAP_H

#include "AxonHillock.h"
#include "BodyComponent.h"
#include "BodyShapeComponent.h"
#include "DendriteBranch.h"
#include "Neuron.h"
#include "Position.h"
#include "math.h"

#include <memory>
#include <vector>
//#include "SensoryReceptor.h"
#include "Effector.h"

class Effector;
class DendriteBouton;
class AxonBouton;
class SensoryReceptor;

class SynapticGap
: public std::enable_shared_from_this<SynapticGap>
, public BodyComponent<Position>
, public BodyShapeComponent
{
    public:
    explicit SynapticGap(const PositionPtr &position, double propRate = BodyComponent<Position>::defaultPropagationRate)
    : BodyComponent<Position>(position, propRate)
    , BodyShapeComponent()
    {
    }
    ~SynapticGap() override = default;
    void initialise();
    void updatePosition(const PositionPtr &newPosition);
    // Method to check if the SynapticGap has been associated
    bool isAssociated() const;
    // Method to set the SynapticGap as associated
    void setAsAssociated();

    void updateFromSensoryReceptor(std::shared_ptr<SensoryReceptor> parentSensoryReceptorPointer);

    [[nodiscard]] std::shared_ptr<SensoryReceptor> getParentSensoryReceptor() const;
    [[nodiscard]] std::shared_ptr<Effector>        getParentEffector() const;
    [[nodiscard]] std::shared_ptr<AxonBouton>      getParentAxonBouton() const;
    [[nodiscard]] std::shared_ptr<DendriteBouton>  getParentDendriteBouton() const;
    [[nodiscard]] const PositionPtr               &getPosition() const;

    void updateFromDendriteBouton(std::shared_ptr<DendriteBouton> parentDendriteBoutonPointer);
    void updateFromAxonBouton(std::shared_ptr<AxonBouton> parentAxonBoutonPointer);
    void updateFromEffector(std::shared_ptr<Effector> &parentEffectorPointer);

    void   updateComponent(double time, double energy);
    double calculateEnergy(double currentTime, double currentEnergyLevel);
    double calculateWaveform(double currentTime) const;
    double propagationTime();

    private:
    bool instanceInitialised = false;  // Initially, the SynapticGap is not initialised
    bool associated          = false;  // Initially, the SynapticGap is not associated with
                                       // a DendriteBouton
    std::shared_ptr<Effector>        parentEffector;
    std::shared_ptr<SensoryReceptor> parentSensoryReceptor;
    std::shared_ptr<AxonBouton>      parentAxonBouton;
    std::shared_ptr<DendriteBouton>  parentDendriteBouton;
    double                           attack;
    double                           decay;
    double                           sustain;
    double                           release;
    double                           frequencyResponse;
    double                           phaseShift;
    double                           previousTime;
    double                           energyLevel;
    double                           componentEnergyLevel;
    double                           minPropagationTime;
    double                           maxPropagationTime;
    double                           lastCallTime;
    int                              callCount;
};
#endif  // SYNAPTICGAP_H
