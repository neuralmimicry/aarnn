#ifndef SENSORYRECENPTOR_H
#define SENSORRECEENPTOR_H

#include <iostream>
#include <memory>
#include <chrono>

#include <vector>
#include "BodyComponent.h"
#include "BodyShapeComponent.h"
#include "Position.h"
//#include "SynapticGap.h"
// #include "Dendrite.h"
// #include "Soma.h"
#include "math.h"

class SensoryReceptor : public std::enable_shared_from_this<SensoryReceptor>, public BodyComponent<Position>, public BodyShapeComponent {
public:
    explicit SensoryReceptor(const PositionPtr &position, double propRate = BodyComponent<Position>::defaultPropagationRate)
    : BodyShapeComponent()
    , BodyComponent(position, propRate)
    {
    }
    ~SensoryReceptor() override = default;

    void initialise() {
        // if (!instanceInitialised) {
        //     setAttack((35 - (rand() % 25)) / 100);
        //     setDecay((35 - (rand() % 25)) / 100);        
        //     setSustain((35 - (rand() % 25)) / 100);
        //     setRelease((35 - (rand() % 25)) / 100);
        //     setFrequencyResponse(rand() % 44100);
        //     setPhaseShift(rand() % 360);
        //     lastCallTime = 0.0;
        //     synapticGap = std::make_shared<SynapticGap>(
        //             std::make_shared<Position>((position->x) + 1, (position->y) + 1, (position->z) + 1));
        //     synapticGap->initialise();
        //     synapticGap->updateFromSensoryReceptor(shared_from_this());
        //     addSynapticGap(synapticGap);
        //     minPropagationRate = (35 - (rand() % 25)) / 100;
        //     maxPropagationRate = (65 + (rand() % 25)) / 100;
        //     instanceInitialised = true;
        //}
    }

    void addSynapticGap(std::shared_ptr<SynapticGap> gap) {
        synapticGaps.emplace_back(std::move(gap));
    }

    void updatePosition(const PositionPtr& newPosition) {
        position = newPosition;
    }

    [[nodiscard]] std::vector<std::shared_ptr<SynapticGap>> getSynapticGaps() const {
        return synapticGaps;
    }

    void setAttack(double newAttack) {
        attack = newAttack;
    }

    void setDecay(double newDecay) {
        decay = newDecay;
    }

    void setSustain(double newSustain) {
        sustain = newSustain;
    }

    void setRelease(double newRelease) {
        release = newRelease;
    }

    void setFrequencyResponse(double newFrequencyResponse) {
        frequencyResponse = newFrequencyResponse;
    }

    void setPhaseShift(double newPhaseShift) {
        phaseShift = newPhaseShift;
    }

    void setEnergyLevel(double newEnergyLevel) {
        energyLevel = newEnergyLevel;
    }

    double calculateEnergy(double currentTime, double currentEnergyLevel) {
        double deltaTime = currentTime - previousTime;
        previousTime = currentTime;
        energyLevel = currentEnergyLevel;

        if (deltaTime < attack) {
            return (deltaTime / attack) * calculateWaveform(currentTime);
        } else if (deltaTime < attack + decay) {
            double decay_time = deltaTime - attack;
            return ((1 - decay_time / decay) * (1 - sustain) + sustain) * calculateWaveform(currentTime);
        } else if (deltaTime < attack + decay + sustain) {
            return sustain * calculateWaveform(currentTime);
        } else {
            double release_time = deltaTime - attack - decay - sustain;
            return (1 - std::max(0.0, release_time / release)) * calculateWaveform(currentTime);
        }
    }

    double calculateWaveform(double currentTime) const {
        return energyLevel * sin(2 * M_PI * frequencyResponse * currentTime + phaseShift);
    }

    double calcPropagationRate() {
        double currentTime = (double) std::clock() / CLOCKS_PER_SEC;
        callCount++;
        double timeSinceLastCall = currentTime - lastCallTime;
        lastCallTime = currentTime;

        double x = 1 / (1 + exp(-callCount / timeSinceLastCall));
        return minPropagationRate + x * (maxPropagationRate - minPropagationRate);
    }

    void updateComponent(double time, double energy) {
        // componentEnergyLevel = calculateEnergy(time, componentEnergyLevel + energy);// Update the component
        // propagationRate = calcPropagationRate();
        // for (auto& synapticGap_id : synapticGaps) {
        //     synapticGap_id->updateComponent(time + position->calcPropagationTime(*synapticGap_id->getPosition(), propagationRate), componentEnergyLevel);
        // }
        // componentEnergyLevel = 0;
    }

    [[nodiscard]] const PositionPtr& getPosition() const {
        return position;
    }

private:
    bool instanceInitialised = false;
    std::vector<std::shared_ptr<SynapticGap>> synapticGaps;
    std::shared_ptr<SynapticGap> synapticGap;
    double attack;
    double decay;
    double sustain;
    double release;
    double frequencyResponse;
    double phaseShift;
    double previousTime;
    double energyLevel;
    double componentEnergyLevel;
    double minPropagationRate;
    double maxPropagationRate;
    double lastCallTime;
    int callCount;
};

#endif //SENSORRECEENPTOR_H