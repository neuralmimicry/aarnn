#ifndef DENDRITE_BRANCH_H_INCLUDED
#define DENDRITE_BRANCH_H_INCLUDED

#include "body_component.h"
#include "fwd_decls.h"
#include "position.h"

#include <memory>

namespace ns_aarnn
{
/**
 * @brief DendriteBranch class
 *
 * @Children @c Dendrites
 * @Parents @c Soma @c Dendrite
 */
class DendriteBranch : public BodyComponent
{
    // construction/destruction
    private:
    explicit DendriteBranch(const Position& position);

    protected:
    void doInitialisation_() override;

    public:
    ~DendriteBranch() override = default;
    static DendriteBranchPtr create(const Position& position);

    // overrides of abstract functions
    static std::string_view name();
    double                  calculatePropagationRate() const override;

    // getters and setters
    void                    addDendrite(DendritePtr dendrite);
    [[nodiscard]] Dendrites getDendrites() const;
    void                    setParentSoma(SomaPtr parentSoma);
    void                    setParentDendrite(DendritePtr parentDendrite);

    private:
    Dendrites onwardDendrites_{};
};
}  // namespace aarnn

#endif  // DENDRITE_BRANCH_H_INCLUDED