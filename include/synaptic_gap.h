
#ifndef SYNAPTIC_GAP_H_INCLUDED
#define SYNAPTIC_GAP_H_INCLUDED

#include "body_component.h"
#include "fwd_decls.h"
#include "position.h"

#include <cmath>
#include <memory>

namespace ns_aarnn
{
class SynapticGap
: public std::enable_shared_from_this<SynapticGap>
, public BodyComponent
{
    private:
    // construction/destruction
    explicit SynapticGap(Position position);

    protected:
    void doInitialisation_() override;

    public:
    static SynapticGapPtr create(Position position);
    ~SynapticGap() override = default;

    // overrides of abstract functions
    static std::string_view name();
    double                  calculatePropagationRate() const override;

    /**
     * Method to check if the SynapticGap has been associated
     * @return true if the synaptic gap is associated, false otherwise
     */
    bool isAssociated() const;

    /**
     * Method to set the SynapticGap as associated.
     */
    void setAsAssociated();

    /**
     * Use the parent sensory receptor to update this.
     * @param parentSensoryReceptor
     */
    void setParentSensoryReceptor(SensoryReceptorPtr parentSensoryReceptor);

    [[nodiscard]] SensoryReceptorPtr getParentSensoryReceptor() const;
    [[nodiscard]] EffectorPtr        getParentEffector() const;
    [[nodiscard]] AxonBoutonPtr      getParentAxonBouton() const;
    [[nodiscard]] DendriteBoutonPtr  getParentDendriteBouton() const;

    void setParentDendriteBouton(DendriteBoutonPtr parentDendriteBouton);
    void setParentAxonBouton(AxonBoutonPtr parentAxonBouton);
    void setParentEffector(EffectorPtr &parentEffector);

    void   updateComponent(double time, double energy);
    double calculateEnergy(double currentTime, double currentEnergyLevel);
    double calculateWaveform(double currentTime) const;
    double propagationTime();

    private:
    bool               associated_{false};  // Initially, the SynapticGap is not associated_ with a DendriteBouton
    EffectorPtr        parentEffector_{};
    SensoryReceptorPtr parentSensoryReceptor_{};
    AxonBoutonPtr      parentAxonBouton_{};
    DendriteBoutonPtr  parentDendriteBouton_{};
    double             attack_{};
    double             decay_{};
    double             sustain_{};
    double             release_{};
    double             frequencyResponse_{};
    double             phaseShift_{};
    double             previousTime_{};
    double             energyLevel_{};
    double             componentEnergyLevel_{};
    double             minPropagationTime_{};
    double             maxPropagationTime_{};
    double             lastCallTime_{};
    int                callCount_{};
};
}  // namespace aarnn

#endif  // SYNAPTIC_GAP_H_INCLUDED
