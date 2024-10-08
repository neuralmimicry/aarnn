
#ifndef DENDRITE_BOUTON_H_INCLUDED
#define DENDRITE_BOUTON_H_INCLUDED

#include "body_component.h"
#include "fwd_decls.h"
#include "position.h"

#include <memory>

namespace ns_aarnn
{
/**
 * @brief DendriteBouton class
 *
 * @Children @c SynapticGap @c SynapticGaps (maintained by the parent Neutron)
 * @Parents @c Neutron @c Dendrite
 */
class DendriteBouton : public BodyComponent
{
    // construction/destruction
    private:
    explicit DendriteBouton(const Position& position);

    protected:
    void doInitialisation_() override;

    public:
    ~DendriteBouton() override = default;
    static DendriteBoutonPtr create(const Position& position);

    // overrides of abstract functions
    static std::string_view name();
    double                  calculatePropagationRate() const override;

    // getters and setters

    /**
     * Add a synaptic gap to the parent Neutron.
     * @param gap the gap to add
     */
    void addSynapticGap(SynapticGapPtr gap);
    void setParentDendrite(DendritePtr parentDendrite);
    void setParentNeutron(NeuronPtr parentNeuron);

    private:
    SynapticGapPtr onwardSynapticGap_;
};
}  // namespace aarnn

#endif  // DENDRITE_BOUTON_H_INCLUDED