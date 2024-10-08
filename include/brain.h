
#ifndef BRAIN_H
#define BRAIN_H

#include "body_component.h"
#include "fwd_decls.h"
#include "parameters.h"
#include "position.h"

#include <thread>

namespace ns_aarnn
{
class Brain
{
    public:
    explicit Brain(size_t numberOfNeurons = 100UL, double proximityThreshold = 0.1)
    : proximityThreshold_(proximityThreshold)
    {
    }

    void initialise(size_t numberOfNeurons         = 1500UL,
                    size_t numberOfVisualInputs    = 2UL,
                    size_t numberOfPixels          = 131UL,
                    size_t numberOfAuditoryInputs  = 2UL,
                    size_t numberOfPhonels         = 44UL,
                    size_t numberOfOlfactoryInputs = 2UL,
                    size_t numberOfScentels        = 4UL,
                    size_t numberOfVocalEffectors  = 2UL,
                    size_t numberOfVocels          = 100UL);

    private:
    void createNeurons_(size_t numberOfNeurons);
    void createVisualReceptors_(size_t numberOfVisualInputs, size_t numberOfPixels);
    void createAuditoryReceptors_(size_t numberOfAuditoryInputs, size_t numberOfPhonels);
    void createOlfactoryReceptors_(size_t numberOfOlfactoryInputs, size_t numberOfScentels);
    void createVocalEffectors_(size_t numberOfVocalEffectors, size_t numberOfVocels);
    void performSynapticAssociation_();

    Neurons                       neurons_{};
    std::vector<SensoryReceptors> visualInputs_{};
    std::vector<SensoryReceptors> audioInputs_{};
    std::vector<SensoryReceptors> olfactoryInputs_{};
    Effectors                     vocalOutputs_{};
    std::vector<std::thread>      threads_{};
    double                        proximityThreshold_{0.1};
    std::atomic<double>           totalPropagationRate_{0.0};
};
}  // namespace aarnn

#endif  // BRAIN_H
