#include "SensoryReceptor.h"
#include "SynapticGap.h"
#include "utils.h"

#include <cmath>
#include <ctime>
#include <cstdlib>
#include <algorithm>

SensoryReceptor::SensoryReceptor(const std::shared_ptr<Position>& position, std::weak_ptr<NeuronalComponent> parent)
        : NeuronalComponent(position, parent)
{
    // Additional initialization if needed
}

void SensoryReceptor::initialise()
{
    NeuronalComponent::initialise(); // Call base class initialise

    if (!instanceInitialised)
    {
        setAttack((35 - (rand() % 25)) / 100.0);
        setDecay((35 - (rand() % 25)) / 100.0);
        setSustain((35 - (rand() % 25)) / 100.0);
        setRelease((35 - (rand() % 25)) / 100.0);
        setFrequencyResponse(rand() % 44100);
        setPhaseShift(rand() % 360);
        lastCallTime = 0.0;

        auto positionPtr = std::make_shared<Position>(position->x + 1, position->y + 1, position->z + 1);
        synapticGap = std::make_shared<SynapticGap>(positionPtr, std::static_pointer_cast<NeuronalComponent>(shared_from_this()));
        synapticGap->initialise();
        synapticGap->updateFromSensoryReceptor(std::static_pointer_cast<SensoryReceptor>(shared_from_this()));
        addSynapticGap(synapticGap);

        minPropagationRate  = (35 - (rand() % 25)) / 100.0;
        maxPropagationRate  = (65 + (rand() % 25)) / 100.0;
        instanceInitialised = true;
    }
}

void SensoryReceptor::addSynapticGap(std::shared_ptr<SynapticGap> gap)
{
    synapticGaps.emplace_back(std::move(gap));
}

std::vector<std::shared_ptr<SynapticGap>> SensoryReceptor::getSynapticGaps() const
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

    if (deltaTime < attack)
    {
        return (deltaTime / attack) * calculateWaveform(currentTime);
    }
    else if (deltaTime < attack + decay)
    {
        double decay_time = deltaTime - attack;
        return ((1 - decay_time / decay) * (1 - sustain) + sustain) * calculateWaveform(currentTime);
    }
    else if (deltaTime < attack + decay + sustain)
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
    double phaseShiftRadians = phaseShift * M_PI / 180.0;
    return energyLevel * sin(2 * M_PI * frequencyResponse * currentTime + phaseShiftRadians);
}

double SensoryReceptor::calcPropagationRate()
{
    double currentTime = static_cast<double>(std::clock()) / CLOCKS_PER_SEC;
    callCount++;
    double timeSinceLastCall = currentTime - lastCallTime;

    if (timeSinceLastCall == 0)
        timeSinceLastCall = 0.0001; // Prevent division by zero

    lastCallTime = currentTime;

    double x = 1 / (1 + exp(-callCount / timeSinceLastCall));
    return minPropagationRate + x * (maxPropagationRate - minPropagationRate);
}

void SensoryReceptor::updateComponent(double time, double energy)
{
    componentEnergyLevel = calculateEnergy(time, componentEnergyLevel + energy); // Update the component
    propagationRate = calcPropagationRate();

    for (auto& synapticGap_id : synapticGaps)
    {
        double propagationTime = position->calcPropagationTime(*synapticGap_id->getPosition(), propagationRate);
        synapticGap_id->updateComponent(time + propagationTime, componentEnergyLevel);
    }

    componentEnergyLevel = 0;
}

void SensoryReceptor::update(double deltaTime) {
    NeuronalComponent::updateEnergy(deltaTime);

    double stimulusToProcess = 0.0;

    // Safely access accumulatedStimulus
    {
        std::lock_guard<std::mutex> lock(receptorMutex);
        stimulusToProcess = accumulatedStimulus;
        accumulatedStimulus = 0.0;
    }

    // Process the stimulus if it exceeds the threshold
    if (stimulusToProcess >= threshold) {
        double energyIncrease = sensitivity * stimulusToProcess;
        energyTopup(energyIncrease);

        // Here you can add code to propagate the signal to connected components
    }

    // Additional updates if necessary
}

void SensoryReceptor::stimulate(double intensity) {
    std::lock_guard<std::mutex> lock(receptorMutex);
    accumulatedStimulus += intensity;
}

void SensoryReceptor::setSensitivity(double newSensitivity) {
    sensitivity = newSensitivity;
}

void SensoryReceptor::setThreshold(double newThreshold) {
    threshold = newThreshold;
}
