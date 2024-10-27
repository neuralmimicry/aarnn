#ifndef AXON_H
#define AXON_H

#include <memory>
#include <vector>
#include "NeuronalComponent.h"
#include "Position.h"

// Forward declarations
class AxonBouton;
class AxonHillock;
class AxonBranch;

class Axon : public NeuronalComponent
{
public:
    explicit Axon(const std::shared_ptr<Position>& position);

    ~Axon() override = default;

    // Override methods from NeuronalComponent
    void initialise() override;
    void update(double deltaTime);

    // Axon-specific methods
    void addBranch(std::shared_ptr<AxonBranch> branch);
    [[nodiscard]] const std::vector<std::shared_ptr<AxonBranch>>& getAxonBranches() const;
    [[nodiscard]] std::shared_ptr<AxonBouton> getAxonBouton() const;
    double calcPropagationTime();
    void updateFromAxonHillock(std::shared_ptr<AxonHillock> parentAxonHillockPointer);
    [[nodiscard]] std::shared_ptr<AxonHillock> getParentAxonHillock() const;
    void updateFromAxonBranch(std::shared_ptr<AxonBranch> parentAxonBranchPointer);
    [[nodiscard]] std::shared_ptr<AxonBranch> getParentAxonBranch() const;
    void setAxonId(int id);
    int getAxonId() const;

private:
    // Member variables
    std::vector<std::shared_ptr<AxonBranch>> axonBranches;
    std::shared_ptr<AxonBouton> onwardAxonBouton;
    std::shared_ptr<AxonHillock> parentAxonHillock;
    std::shared_ptr<AxonBranch> parentAxonBranch;
    int axonId;
};

#endif  // AXON_H
