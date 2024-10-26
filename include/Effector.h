#ifndef EFFECTOR_H
#define EFFECTOR_H

#include "NeuronalComponent.h"
#include "Position.h"
#include "SynapticGap.h"

#include <memory>
#include <vector>

class SynapticGap;
class Position;

class Effector : public NeuronalComponent
{
public:
    explicit Effector(const std::shared_ptr<Position>& position);

    ~Effector() override = default;

    void initialise() override;
    void addSynapticGap(std::shared_ptr<SynapticGap> synapticGap);

    [[nodiscard]] std::vector<std::shared_ptr<SynapticGap>> getSynapticGaps() const;

private:
    bool instanceInitialised = false;
    std::vector<std::shared_ptr<SynapticGap>> synapticGaps;
};

#endif  // EFFECTOR_H
