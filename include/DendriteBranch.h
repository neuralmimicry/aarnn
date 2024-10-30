#ifndef DENDRITEBRANCH_H
#define DENDRITEBRANCH_H

#include "NeuronalComponent.h"
#include "Dendrite.h"
#include "Position.h"
#include "Soma.h"
#include "SynapticGap.h"

#include <memory>
#include <vector>

class Soma;
class Dendrite;

class DendriteBranch : public NeuronalComponent
{
public:
    explicit DendriteBranch(const std::shared_ptr<Position>& position, std::weak_ptr<NeuronalComponent> parent);

    ~DendriteBranch() override = default;
    void initialise() override;
    void connectDendrite(std::shared_ptr<Dendrite> dendrite);
    [[nodiscard]] const std::vector<std::shared_ptr<Dendrite>>& getDendrites() const;
    void updateFromSoma(std::shared_ptr<Soma> parentSomaPointer);
    [[nodiscard]] std::shared_ptr<Soma> getParentSoma() const;
    void updateFromDendrite(std::shared_ptr<Dendrite> parentDendritePointer);
    [[nodiscard]] std::shared_ptr<Dendrite> getParentDendrite() const;
    void update(double deltaTime);
    void setDendriteBranchId(int id);
    int getDendriteBranchId() const;

private:
    int dendriteBranchId = -1;
    bool instanceInitialised = false;  // Initially, the Dendrite is not initialised
    std::vector<std::shared_ptr<Dendrite>> onwardDendrites;
    std::shared_ptr<Soma> parentSoma;
    std::shared_ptr<Dendrite> parentDendrite;
};

#endif // DENDRITEBRANCH_H
