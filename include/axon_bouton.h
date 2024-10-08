#ifndef AXON_BOUTON_H_INCLUDED
#define AXON_BOUTON_H_INCLUDED

#include "body_component.h"
#include "fwd_decls.h"
#include "position.h"

#include <memory>

namespace ns_aarnn
{
/**
 * @brief AxonBouton class.
 *
 * @Children -NONE-
 * @Parents @c Neuron @c Axon
 */
class AxonBouton : public BodyComponent
{
    // construction/destruction
    private:
    explicit AxonBouton(const Position& position);

    protected:
    void doInitialisation_() override;

    public:
    ~AxonBouton() override = default;
    static AxonBoutonPtr create(const Position& position);

    // overrides of abstract functions
    static std::string_view name();
    double                  calculatePropagationRate() const override;

    // getters and setters
    void setParentNeuron(NeuronPtr parentNeuron);
    void setParentAxon(AxonPtr parentAxon);
};
}  // namespace aarnn

#endif  // AXON_BOUTON_H_INCLUDED
