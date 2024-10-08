#ifndef DENDRITE_H_INCLUDED
#define DENDRITE_H_INCLUDED

#include "body_component.h"
#include "fwd_decls.h"
#include "position.h"

#include <iostream>

namespace ns_aarnn
{
/**
 * @brief Dendrite class.
 *
 * @Children @c DendriteBranches
 * @Parents @c DendriteBranch
 */
class Dendrite : public BodyComponent
{
    // construction/destruction
    private:
    explicit Dendrite(const Position& position);

    protected:
    void doInitialisation_() override;

    public:
    ~Dendrite() override = default;
    static DendritePtr create(Position position);

    // overrides of abstract functions
    static std::string_view name();
    double                  calculatePropagationRate() const override;

    // getters and setters
    void                    addBranch(DendriteBranchPtr branch);
    const DendriteBranches& getDendriteBranches() const;
    void                    setParentDendriteBranch(DendriteBranchPtr parentDendriteBranch);

    private:
    DendriteBranches dendriteBranches_{};
};
}  // namespace aarnn

#endif  // DENDRITE_H_INCLUDED
