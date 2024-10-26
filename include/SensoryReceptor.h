#ifndef SENSORYRECEPTOR_H
#define SENSORYRECEPTOR_H

#include "NeuronalComponent.h"
#include "Position.h"
#include "SynapticGap.h"

#include <chrono>
#include <iostream>
#include <memory>
#include <vector>
#include <cmath>

class SynapticGap;

class SensoryReceptor : public NeuronalComponent
{
public:
    explicit SensoryReceptor(const std::shared_ptr<Position>& position);

    ~SensoryReceptor() override = default;

    void initialise() override;

    void addSynapticGap(std::shared_ptr<SynapticGap> gap);

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
    double                                    propagationRate{};
};

#endif  // SENSORYRECEPTOR_H
