#ifndef AXON_BRANCH_H_INCLUDED
#define AXON_BRANCH_H_INCLUDED

#include "axon.h"
#include "body_component.h"
#include "position.h"

#include <memory>

namespace ns_aarnn
{
/**
 * @brief AxonBranch class.
 *
 * @Children @c Axons
 * @Parents @c Axon
 */
class AxonBranch : public BodyComponent
{
    private:
    // overrides of abstract functions
    explicit AxonBranch(const Position &position);

    protected:
    void doInitialisation_() override;

    public:
    ~AxonBranch() override = default;
    static AxonBranchPtr create(const Position &position);

    // overrides of abstract functions
    static std::string_view name();
    double                  calculatePropagationRate() const override;

    // getters and setters
    void                       addAxon(AxonPtr axon);
    void                       setParentAxon(AxonPtr parentAxon);
    [[nodiscard]] const Axons &getAxons() const;

    private:
    Axons onwardAxons_{};
};
}  // namespace aarnn
#endif  // AXON_BRANCH_H_INCLUDED