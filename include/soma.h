#ifndef SOMA_H_INCLUDED
#define SOMA_H_INCLUDED

#include "body_component.h"
#include "fwd_decls.h"
#include "position.h"

#include <memory>

namespace ns_aarnn
{
class Soma : public BodyComponent
{
    private:
    // construction/destruction
    explicit Soma(const Position &position);

    protected:
    void doInitialisation_() override;

    public:
    static SomaPtr create(const Position &position);
    ~Soma() override = default;

    // overrides of abstract functions
    static std::string_view name();
    double                  calculatePropagationRate() const override;

    // getters and setters
    [[nodiscard]] AxonHillockPtr          getAxonHillock() const;
    void                                  addDendriteBranch(DendriteBranchPtr dendriteBranch);
    [[nodiscard]] const DendriteBranches &getDendriteBranches() const;
    void                                  setParentNeuron(NeuronPtr parentNeuron);
    [[nodiscard]] NeuronPtr               getParentNeuron() const;

    private:
    SynapticGaps     synapticGaps_{};
    DendriteBranches dendriteBranches_{};
    AxonHillockPtr   onwardAxonHillock_{};
    NeuronPtr        parentNeuron_{};
};
}  // namespace aarnn

#endif  // SOMA_H_INCLUDED
