#ifndef EFFECTOR_H_INCLUDED
#define EFFECTOR_H_INCLUDED

#include "body_component.h"
#include "position.h"
#include "synaptic_gap.h"

#include <memory>

namespace ns_aarnn
{
/**
 * @brief Effector class
 *
 * @Children @c SynapticGaps
 * @Parents -NONE-
 */
class Effector : public BodyComponent
{
    // construction/destruction
    private:
    explicit Effector(Position position);

    protected:
    void doInitialisation_() override;

    public:
    static EffectorPtr create(Position position);
    ~Effector() override = default;

    // overrides of abstract functions
    static std::string_view name();
    double                  calculatePropagationRate() const override;

    // getters and setters
    void                       addSynapticGap(SynapticGapPtr synapticGap);
    [[nodiscard]] SynapticGaps getSynapticGaps() const;

    private:
    SynapticGaps synapticGaps_{};
};

}  // namespace aarnn

#endif  // EFFECTOR_H_INCLUDED
