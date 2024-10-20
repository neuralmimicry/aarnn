#include "SensoryReceptor.h"
#include "SynapticGap.h"
#include "utils.h"

#include <cmath>

void SensoryReceptor::initialise()
{
    if(!instanceInitialised)
    {
        setAttack((35 - (rand() % 25)) / 100);
        setDecay((35 - (rand() % 25)) / 100);
        setSustain((35 - (rand() % 25)) / 100);
        setRelease((35 - (rand() % 25)) / 100);
        setFrequencyResponse(rand() % 44100);
        setPhaseShift(rand() % 360);
        lastCallTime            = 0.0;
        PositionPtr positionPtr = std::make_shared<Position>((position->x) + 1, (position->y) + 1, (position->z) + 1);
        this->synapticGap             = std::make_shared<SynapticGap>(positionPtr);
        this->synapticGap->initialise();
        this->synapticGap->updateFromSensoryReceptor(shared_from_this());
        addSynapticGap(this->synapticGap);
        minPropagationRate  = (35 - (rand() % 25)) / 100;
        maxPropagationRate  = (65 + (rand() % 25)) / 100;
        instanceInitialised = true;
    }
}

void SensoryReceptor::addSynapticGap(std::shared_ptr<SynapticGap> gap)
{
    synapticGaps.emplace_back(std::move(gap));
}

void SensoryReceptor::updatePosition(const PositionPtr &newPosition)
{
    position = newPosition;
}

[[nodiscard]] std::vector<std::shared_ptr<SynapticGap>> SensoryReceptor::getSynapticGaps() const
{
    return synapticGaps;
}

void SensoryReceptor::setAttack(double newAttack)
{
    attack = newAttack;
}

void SensoryReceptor::setDecay(double newDecay)
{
    decay = newDecay;
}

void SensoryReceptor::setSustain(double newSustain)
{
    sustain = newSustain;
}

void SensoryReceptor::setRelease(double newRelease)
{
    release = newRelease;
}

void SensoryReceptor::setFrequencyResponse(double newFrequencyResponse)
{
    frequencyResponse = newFrequencyResponse;
}

void SensoryReceptor::setPhaseShift(double newPhaseShift)
{
    phaseShift = newPhaseShift;
}

void SensoryReceptor::setEnergyLevel(double newEnergyLevel)
{
    energyLevel = newEnergyLevel;
}

double SensoryReceptor::calculateEnergy(double currentTime, double currentEnergyLevel)
{
    double deltaTime = currentTime - previousTime;
    previousTime     = currentTime;
    energyLevel      = currentEnergyLevel;

    if(deltaTime < attack)
    {
        return (deltaTime / attack) * calculateWaveform(currentTime);
    }
    else if(deltaTime < attack + decay)
    {
        double decay_time = deltaTime - attack;
        return ((1 - decay_time / decay) * (1 - sustain) + sustain) * calculateWaveform(currentTime);
    }
    else if(deltaTime < attack + decay + sustain)
    {
        return sustain * calculateWaveform(currentTime);
    }
    else
    {
        double release_time = deltaTime - attack - decay - sustain;
        return (1 - std::max(0.0, release_time / release)) * calculateWaveform(currentTime);
    }
}

double SensoryReceptor::calculateWaveform(double currentTime) const
{
    return energyLevel * sin(2 * M_PI * frequencyResponse * currentTime + phaseShift);
}

double SensoryReceptor::calcPropagationRate()
{
    double currentTime = (double)std::clock() / CLOCKS_PER_SEC;
    callCount++;
    double timeSinceLastCall = currentTime - lastCallTime;
    lastCallTime             = currentTime;

    double x = 1 / (1 + exp(-callCount / timeSinceLastCall));
    return minPropagationRate + x * (maxPropagationRate - minPropagationRate);
}

void SensoryReceptor::updateComponent(double time, double energy)
{
    // componentEnergyLevel = calculateEnergy(time, componentEnergyLevel + energy);// Update the component
    // propagationRate = calcPropagationRate();
    // for (auto& synapticGap_id : synapticGaps) {
    //     synapticGap_id->updateComponent(time + position->calcPropagationTime(*synapticGap_id->getPosition(),
    //     propagationRate), componentEnergyLevel);
    // }
    // componentEnergyLevel = 0;
}

[[nodiscard]] const PositionPtr &SensoryReceptor::getPosition() const
{
    return position;
}