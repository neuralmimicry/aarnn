#include "synaptic_gap.h"

#include "position.h"
#include "sensory_receptor.h"

namespace ns_aarnn
{
SynapticGap::SynapticGap(Position position)
: BodyComponent(nextID<SynapticGap>(), position)
{
    setNextIDFunction(nextID<SynapticGap>);
}

SynapticGapPtr SynapticGap::create(Position position)
{
    return std::shared_ptr<SynapticGap>(new SynapticGap(position));
}

std::string_view SynapticGap::name()
{
    return std::string_view{"SynapticGap"};
}

void SynapticGap::doInitialisation_()
{
    // Nothing to do
}

double SynapticGap::calculatePropagationRate() const
{
    // TODO: default propagation rate should be overridden
    return getPropagationRate();
}

// Method to check if the SynapticGap has been associated_
bool SynapticGap::isAssociated() const
{
    return associated_;
}

// Method to set the SynapticGap as associated_
void SynapticGap::setAsAssociated()
{
    associated_ = true;
}

void SynapticGap::setParentSensoryReceptor(SensoryReceptorPtr parentSensoryReceptor)
{
    parentSensoryReceptor_ = std::move(parentSensoryReceptor);
}

void SynapticGap::setParentEffector(EffectorPtr &parentEffector)
{
    parentEffector_ = std::move(parentEffector);
}

void SynapticGap::setParentAxonBouton(AxonBoutonPtr parentAxonBouton)
{
    parentAxonBouton_ = std::move(parentAxonBouton);
}

void SynapticGap::setParentDendriteBouton(DendriteBoutonPtr parentDendriteBouton)
{
    parentDendriteBouton_ = std::move(parentDendriteBouton);
}

void SynapticGap::updateComponent(double time, double energy)
{
    componentEnergyLevel_ = calculateEnergy(time, componentEnergyLevel_ + energy);  // Update the component
    // for (auto& synapticGap_id : synapticGaps_) {
    //     synapticGap_id->updateComponent(time + propagationTime(), componentEnergyLevel_);
    // }
}

double SynapticGap::calculateEnergy(double currentTime, double currentEnergyLevel)
{
    double deltaTime = currentTime - previousTime_;
    previousTime_    = currentTime;
    energyLevel_     = currentEnergyLevel;

    if(deltaTime < attack_)
    {
        return (deltaTime / attack_) * calculateWaveform(currentTime);
    }
    else if(deltaTime < attack_ + decay_)
    {
        double decay_time = deltaTime - attack_;
        return ((1 - decay_time / decay_) * (1 - sustain_) + sustain_) * calculateWaveform(currentTime);
    }
    else if(deltaTime < attack_ + decay_ + sustain_)
    {
        return sustain_ * calculateWaveform(currentTime);
    }
    else
    {
        double release_time = deltaTime - attack_ - decay_ - sustain_;
        return (1 - std::max(0.0, release_time / release_)) * calculateWaveform(currentTime);
    }
}

double SynapticGap::calculateWaveform(double currentTime) const
{
    return energyLevel_ * sin(2 * M_PI * frequencyResponse_ * currentTime + phaseShift_);
}

double SynapticGap::propagationTime()
{
    double currentTime = (double)std::clock() / CLOCKS_PER_SEC;
    callCount_++;
    double timeSinceLastCall = currentTime - lastCallTime_;
    lastCallTime_            = currentTime;

    double x = 1 / (1 + exp(-callCount_ / timeSinceLastCall));
    return minPropagationTime_ + x * (maxPropagationTime_ - minPropagationTime_);
}

[[nodiscard]] SensoryReceptorPtr SynapticGap::getParentSensoryReceptor() const
{
    return parentSensoryReceptor_;
}

[[nodiscard]] EffectorPtr SynapticGap::getParentEffector() const
{
    return parentEffector_;
}

[[nodiscard]] AxonBoutonPtr SynapticGap::getParentAxonBouton() const
{
    return parentAxonBouton_;
}

[[nodiscard]] DendriteBoutonPtr SynapticGap::getParentDendriteBouton() const
{
    return parentDendriteBouton_;
}
}  // namespace aarnn
