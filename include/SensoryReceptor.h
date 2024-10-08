#ifndef SENSORYRECEPTOR_H
#define SENSORYRECEPTOR_H

#include "BodyComponent.h"
#include "BodyShapeComponent.h"
#include "Position.h"

#include <chrono>
#include <iostream>
#include <memory>
#include <vector>

class SynapticGap;

class SensoryReceptor
: public std::enable_shared_from_this<SensoryReceptor>
, public BodyComponent<Position>
, public BodyShapeComponent
{
    public:
    explicit SensoryReceptor(const PositionPtr &position,
                             double             propRate = BodyComponent<Position>::defaultPropagationRate)
    : BodyComponent<Position>(position, propRate)
    , BodyShapeComponent()
    {
    }
    ~SensoryReceptor() override = default;

    void initialise();

    void addSynapticGap(std::shared_ptr<SynapticGap> gap);

    void updatePosition(const PositionPtr &newPosition);

    [[nodiscard]] std::vector<std::shared_ptr<SynapticGap>> getSynapticGaps() const;

    void setAttack(double newAttack);
    void setDecay(double newDecay);

    void setSustain(double newSustain);
    void setRelease(double newRelease);

    void setFrequencyResponse(double newFrequencyResponse);

    void   setPhaseShift(double newPhaseShift);
    void   setEnergyLevel(double newEnergyLevel);
    double calculateEnergy(double currentTime, double currentEnergyLevel);

    double calculateWaveform(double currentTime) const;
    double calcPropagationRate();
    void   updateComponent(double time, double energy);

    [[nodiscard]] const PositionPtr &getPosition() const;

    private:
    bool                                      instanceInitialised = false;
    std::vector<std::shared_ptr<SynapticGap>> synapticGaps;
    std::shared_ptr<SynapticGap>              synapticGap;
    double                                    attack{};
    double                                    decay{};
    double                                    sustain{};
    double                                    release{};
    double                                    frequencyResponse{};
    double                                    phaseShift{};
    double                                    previousTime{};
    double                                    energyLevel{};
    double                                    componentEnergyLevel{};
    double                                    minPropagationRate{};
    double                                    maxPropagationRate{};
    double                                    lastCallTime{};
    int                                       callCount{};
};

#endif  // SENSORYRECEPTOR_H
