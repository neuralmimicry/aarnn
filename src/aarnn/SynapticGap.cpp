#include "SynapticGap.h"

void SynapticGap::initialise()
{
    if(!instanceInitialised)
    {
        instanceInitialised = true;
    }
}

void SynapticGap::updatePosition(const PositionPtr &newPosition)
{
    position = newPosition;
}

// Method to check if the SynapticGap has been associated
bool SynapticGap::isAssociated() const
{
    return associated;
}
// Method to set the SynapticGap as associated
void SynapticGap::setAsAssociated()
{
    associated = true;
}

void SynapticGap::updateFromSensoryReceptor(std::shared_ptr<SensoryReceptor> parentSensoryReceptorPointer)
{
    this->parentSensoryReceptor = std::move(parentSensoryReceptorPointer);
}

void SynapticGap::updateFromEffector(std::shared_ptr<Effector> &parentEffectorPointer)
{
    this->parentEffector = std::move(parentEffectorPointer);
}

void SynapticGap::updateFromAxonBouton(std::shared_ptr<AxonBouton> parentAxonBoutonPointer)
{
    this->parentAxonBouton = std::move(parentAxonBoutonPointer);
}

void SynapticGap::updateFromDendriteBouton(std::shared_ptr<DendriteBouton> parentDendriteBoutonPointer)
{
    this->parentDendriteBouton = std::move(parentDendriteBoutonPointer);
}

void SynapticGap::updateComponent(double time, double energy)
{
    componentEnergyLevel = calculateEnergy(time, componentEnergyLevel + energy);  // Update the component
    // for (auto& synapticGap_id : synapticGaps) {
    //     synapticGap_id->updateComponent(time + propagationTime(), componentEnergyLevel);
    // }
}

double SynapticGap::calculateEnergy(double currentTime, double currentEnergyLevel)
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

double SynapticGap::calculateWaveform(double currentTime) const
{
    return energyLevel * sin(2 * M_PI * frequencyResponse * currentTime + phaseShift);
}

double SynapticGap::propagationTime()
{
    double currentTime = (double)std::clock() / CLOCKS_PER_SEC;
    callCount++;
    double timeSinceLastCall = currentTime - lastCallTime;
    lastCallTime             = currentTime;

    double x = 1 / (1 + exp(-callCount / timeSinceLastCall));
    return minPropagationTime + x * (maxPropagationTime - minPropagationTime);
}

[[nodiscard]] std::shared_ptr<SensoryReceptor> SynapticGap::getParentSensoryReceptor() const
{
    return this->parentSensoryReceptor;
}
[[nodiscard]] std::shared_ptr<Effector> SynapticGap::getParentEffector() const
{
    return this->parentEffector;
}
[[nodiscard]] std::shared_ptr<AxonBouton> SynapticGap::getParentAxonBouton() const
{
    return this->parentAxonBouton;
}
[[nodiscard]] std::shared_ptr<DendriteBouton> SynapticGap::getParentDendriteBouton() const
{
    return this->parentDendriteBouton;
}
[[nodiscard]] const PositionPtr &SynapticGap::getPosition() const
{
    return position;
}
