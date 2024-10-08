#ifndef NEURON_H_INCLUDED
#define NEURON_H_INCLUDED

#include "body_component.h"
#include "fwd_decls.h"
#include "position.h"
#include "string_utils.h"
#include "thread_safe_queue.h"

#include <iostream>
#include <memory>

namespace ns_aarnn
{
/**
 * @brief Neuron class
 *
 * @Children @c AxonBoutons, @c SynapticGaps (to Axons), @c SynapticGaps (to Dendrites), @c DendriteBoutons
 * @Parents -NONE-
 */
class Neuron : public BodyComponent
{
    private:
    // construction/destruction
    explicit Neuron(Position position);

    protected:
    void doInitialisation_() override;

    public:
    ~Neuron() override = default;
    static NeuronPtr create(const Position &position);

    // overrides of abstract functions
    static std::string_view name();
    double                  calculatePropagationRate() const override;

    // getters and setters
    [[nodiscard]] SynapticGaps    &getSynapticGapsAxon();
    [[nodiscard]] DendriteBoutons &getDendriteBoutons();
    void                           addSynapticGapAxon(SynapticGapPtr synapticGap);
    void                           addSynapticGapDendrite(SynapticGapPtr synapticGap);

    // traversal
    void storeAllSynapticGapsAxon();
    void storeAllSynapticGapsDendrite();

    /**
     * @brief Compute the propagation rate of a neuron.
     * @param neuron Pointer to the Neuron object for which the propagation rate is to be calculated.
     */
    void computePropagationRate(std::atomic<double> &totalPropagationRate) const;

    void associateSynapticGap(const NeuronPtr &neuron2, double proximityThreshold);

    private:
    AxonBoutons     axonBoutons_{};
    SynapticGaps    synapticGaps_Axon_{};
    SynapticGaps    synapticGaps_Dendrite_{};
    DendriteBoutons dendriteBoutons_{};

    void traverseAxonsForStorage(const AxonPtr &axon);
    void traverseDendritesForStorage(const DendriteBranches &dendriteBranches);
};
}  // namespace aarnn

#endif  // NEURON_H_INCLUDED
