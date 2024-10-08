#ifndef SENSORY_RECEPTOR_H_INCLUDED
#define SENSORY_RECEPTOR_H_INCLUDED

#include "body_component.h"
#include "fwd_decls.h"
#include "position.h"

#include <chrono>
#include <iostream>
#include <memory>

namespace ns_aarnn
{
/**
 * @brief SensoryReceptor class
 *
 * @Children
 * @Parents
 */
class SensoryReceptor : public BodyComponent
{
    private:
    // construction/destruction
    explicit SensoryReceptor(Position position);

    protected:
    void doInitialisation_() override;

    public:
    static SensoryReceptorPtr create(Position position);
    ~SensoryReceptor() override = default;

    // overrides of abstract functions
    static std::string_view name();
    double                  calculatePropagationRate() const override;

    // getters and setters
    void                       addSynapticGap(SynapticGapPtr gap);
    [[nodiscard]] SynapticGaps getSynapticGaps() const;
    void                       setAttack(double newAttack);
    void                       setDecay(double newDecay);
    void                       setSustain(double newSustain);
    void                       setRelease(double newRelease);
    void                       setFrequencyResponse(double newFrequencyResponse);
    void                       setPhaseShift(double newPhaseShift);
    void                       setEnergyLevel(double newEnergyLevel);
    double                     calculateEnergy(double currentTime, double currentEnergyLevel);

    double calculateWaveform(double currentTime) const;
    double calcPropagationRate();
    void   updateComponent(double time, double energy);
    void   associateSynapticGap(NeuronPtr neuron, double proximityThreshold);

    private:
    SynapticGaps   synapticGaps_{};
    SynapticGapPtr synapticGap_{};
    double         attack_{};
    double         decay_{};
    double         sustain_{};
    double         release_{};
    double         frequencyResponse_{};
    double         phaseShift_{};
    double         previousTime_{};
    double         energyLevel_{};
    double         componentEnergyLevel_{};
    double         minPropagationRate_{};
    double         maxPropagationRate_{};
    double         lastCallTime_{};
    int            callCount_{};
};
}  // namespace aarnn

#endif  // SENSORY_RECEPTOR_H_INCLUDED
