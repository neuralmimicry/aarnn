#ifndef DENDRITE_H
#define DENDRITE_H

#include "NeuronalComponent.h"
#include "DendriteBouton.h"
#include "DendriteBranch.h"
#include "Position.h"

#include <memory>
#include <vector>

class DendriteBouton;
class DendriteBranch;

class Dendrite : public NeuronalComponent
{
private:
    bool instanceInitialised = false;  // Initially, the Dendrite is not initialised
    std::vector<std::shared_ptr<DendriteBranch>> dendriteBranches;
    std::shared_ptr<DendriteBouton> dendriteBouton;
    std::shared_ptr<DendriteBranch> parentDendriteBranch;
    int dendriteId = -1;

public:
    explicit Dendrite(const std::shared_ptr<Position>& position, std::weak_ptr<NeuronalComponent> parent);

    ~Dendrite() override = default;

    void initialise() override;
    void addBranch(std::shared_ptr<DendriteBranch> branch);
    [[nodiscard]] const std::vector<std::shared_ptr<DendriteBranch>>& getDendriteBranches() const;
    [[nodiscard]] std::shared_ptr<DendriteBouton> getDendriteBouton() const;
    void updateFromDendriteBranch(std::shared_ptr<DendriteBranch> parentDendriteBranchPointer);
    [[nodiscard]] std::shared_ptr<DendriteBranch> getParentDendriteBranch() const;
    std::shared_ptr<DendriteBouton> getDendriteBoutons();
    void update(double deltaTime);
    void setDendriteId(int id);
    int getDendriteId() const;
};

#endif  // DENDRITE_H
