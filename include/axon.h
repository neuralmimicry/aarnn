#ifndef AXON_H_INCLUDED
#define AXON_H_INCLUDED

#include "body_component.h"
#include "fwd_decls.h"
#include "position.h"

namespace ns_aarnn
{
/**
 * @brief Axon class.
 *
 * @Children @c AxonBranches
 * @Parents @c AxonHillock @c AxonBranch
 */
class Axon : public BodyComponent
{
    // construction/destruction
    private:
    explicit Axon(const Position& position);

    protected:
    void doInitialisation_() override;

    public:
    static AxonPtr create(const Position& position);
    ~Axon() override = default;

    // overrides of abstract functions
    static std::string_view name();
    double                  calculatePropagationRate() const override;

    // getters and setters
    void                              addBranch(AxonBranchPtr branch);
    [[nodiscard]] const AxonBranches& getAxonBranches() const;
    void                              setParentAxonHillock(AxonHillockPtr parentAxonHillock);
    void                              setParentAxonBranch(AxonBranchPtr parentAxonBranchPointer);

    private:
    AxonBranches axonBranches_{};
};
}  // namespace aarnn

#endif  // AXON_H_INCLUDED
