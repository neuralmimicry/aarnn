#ifndef AXON_HILLOCK_H_INCLUDED
#define AXON_HILLOCK_H_INCLUDED

#include "body_component.h"
#include "fwd_decls.h"
#include "position.h"

#include <memory>

namespace ns_aarnn
{
/**
 * @brief AxonHillock class.
 *
 * @Children -None-
 * @Parents @c Soma
 */
class AxonHillock : public BodyComponent
{
    // overrides of abstract functions
    private:
    explicit AxonHillock(const Position &position);

    protected:
    void doInitialisation_() override;

    public:
    ~AxonHillock() override = default;
    static AxonHillockPtr create(const Position &position);

    // overrides of abstract functions
    static std::string_view name();
    double                  calculatePropagationRate() const override;

    // getters and setters
    void setParentSoma(SomaPtr pParentSoma);

    private:
};
}  // namespace aarnn

#endif  // AXON_HILLOCK_H_INCLUDED