#include "sensory_receptor.h"

#include "dendrite_bouton.h"
#include "neuron.h"
#include "position.h"
#include "rand_utils.h"
#include "synaptic_gap.h"

#include <cmath>

namespace ns_aarnn
{
SensoryReceptor::SensoryReceptor(Position position) : BodyComponent(nextID<SensoryReceptor>(), position)
{
    setNextIDFunction(nextID<SensoryReceptor>);
}

SensoryReceptorPtr SensoryReceptor::create(Position position)
{
    return std::shared_ptr<SensoryReceptor>(new SensoryReceptor(position));
}

std::string_view SensoryReceptor::name()
{
    return std::string_view{"SensoryReceptor"};
}

void SensoryReceptor::doInitialisation_()
{
    // setAttack((double)(35 - (rand() % 25)) / 100.0);
    setAttack(rand_val(0.10, 0.35));
    // setDecay((35 - (rand() % 25)) / 100);
    setDecay(rand_val(0.10, 0.35));
    // setSustain((35 - (rand() % 25)) / 100);
    setSustain(rand_val(0.10, 0.35));
    // setRelease((35 - (rand() % 25)) / 100);
    setRelease(rand_val(0.10, 0.35));
    // setFrequencyResponse(rand() % 44100);
    setFrequencyResponse(rand_val(0.0, 44100.0));
    // setPhaseShift(rand() % 360);
    setPhaseShift(rand_val(0.0, 360.0));
    lastCallTime_ = 0.0;
    synapticGap_  = SynapticGap::create(Position{x() + 1.0, y() + 1.0, z() + 1.0});
    synapticGap_->initialise();
    synapticGap_->setParentSensoryReceptor(this_shared<SensoryReceptor>());
    addSynapticGap(synapticGap_);
    // minPropagationRate_  = (35 - (rand() % 25)) / 100;
    minPropagationRate_ = rand_val(0.10, 0.35);
    // maxPropagationRate_  = (65 + (rand() % 25)) / 100;
    maxPropagationRate_ = rand_val(0.40, 0.65);
}

double SensoryReceptor::calculatePropagationRate() const
{
    // TODO: default propagation rate should be overridden
    return getPropagationRate();
}

void SensoryReceptor::associateSynapticGap(NeuronPtr neuron, double proximityThreshold)
{
    // Iterate over all SynapticGaps of receptor
    for(auto& gap: getSynapticGaps())
    {
        // If the bouton has a synaptic gap, and it is not associated_ yet
        if(gap && !gap->isAssociated())
        {
            // Iterate over all DendriteBoutons in neuron
            for(auto& dendriteBouton: neuron->getDendriteBoutons())
            {
                spdlog::info("Checking gap {} with dendrite bouton {}", toString(gap), toString(dendriteBouton));

                // If the distance between the gap and the dendriteBouton_ is below the proximity threshold
                if(gap->distanceTo(*dendriteBouton) < proximityThreshold)
                {
                    // Associate the synaptic gap with the dendriteBouton_
                    dendriteBouton->addSynapticGap(gap);
                    // Set the SynapticGap as associated_
                    gap->setAsAssociated();
                    spdlog::info("Associated gap {} with dendrite bouton {}", toString(gap), toString(dendriteBouton));
                    // No need to check other DendriteBoutons once the gap is associated_
                    break;
                }
            }
        }
    }
}

void SensoryReceptor::addSynapticGap(SynapticGapPtr gap)
{
    synapticGaps_.emplace_back(std::move(gap));
}

[[nodiscard]] SynapticGaps SensoryReceptor::getSynapticGaps() const
{
    return synapticGaps_;
}

void SensoryReceptor::setAttack(double newAttack)
{
    attack_ = newAttack;
}

void SensoryReceptor::setDecay(double newDecay)
{
    decay_ = newDecay;
}

void SensoryReceptor::setSustain(double newSustain)
{
    sustain_ = newSustain;
}

void SensoryReceptor::setRelease(double newRelease)
{
    release_ = newRelease;
}

void SensoryReceptor::setFrequencyResponse(double newFrequencyResponse)
{
    frequencyResponse_ = newFrequencyResponse;
}

void SensoryReceptor::setPhaseShift(double newPhaseShift)
{
    phaseShift_ = newPhaseShift;
}

void SensoryReceptor::setEnergyLevel(double newEnergyLevel)
{
    energyLevel_ = newEnergyLevel;
}

double SensoryReceptor::calculateEnergy(double currentTime, double currentEnergyLevel)
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

double SensoryReceptor::calculateWaveform(double currentTime) const
{
    return energyLevel_ * sin(2 * M_PI * frequencyResponse_ * currentTime + phaseShift_);
}

double SensoryReceptor::calcPropagationRate()
{
    double currentTime = (double)std::clock() / CLOCKS_PER_SEC;
    callCount_++;
    double timeSinceLastCall = currentTime - lastCallTime_;
    lastCallTime_            = currentTime;

    double x = 1 / (1 + exp(-callCount_ / timeSinceLastCall));
    return minPropagationRate_ + x * (maxPropagationRate_ - minPropagationRate_);
}

void SensoryReceptor::updateComponent(double time, double energy)
{
    // componentEnergyLevel_ = calculateEnergy(time, componentEnergyLevel_ + energy);// Update the component
    // propagationRate = calcPropagationRate();
    // for (auto& synapticGap_id : synapticGaps_) {
    //     synapticGap_id->updateComponent(time + position->calcPropagationTime(*synapticGap_id->getPosition(),
    //     propagationRate), componentEnergyLevel_);
    // }
    // componentEnergyLevel_ = 0;
}

}  // namespace aarnn
