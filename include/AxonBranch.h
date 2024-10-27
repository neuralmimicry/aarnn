#ifndef AXONBRANCH_H
#define AXONBRANCH_H

#include <memory>
#include <vector>
#include "NeuronalComponent.h"
#include "Position.h"

// Forward declarations
class Axon;

class AxonBranch : public NeuronalComponent
{
public:
    explicit AxonBranch(const std::shared_ptr<Position>& position);

    ~AxonBranch() override = default;

    // Override methods from NeuronalComponent
    void initialise() override;
    void update(double deltaTime);

    // AxonBranch-specific methods
    void connectAxon(std::shared_ptr<Axon> axon);
    [[nodiscard]] const std::vector<std::shared_ptr<Axon>>& getAxons() const;
    void updateFromAxon(std::shared_ptr<Axon> parentPointer);
    [[nodiscard]] std::shared_ptr<Axon> getParentAxon() const;
    void setAxonBranchId(int id);
    int getAxonBranchId() const;

private:
    bool                               instanceInitialised = false;
    std::vector<std::shared_ptr<Axon>> onwardAxons;
    std::shared_ptr<Axon>              parentAxon;
    int                                axonBranchId;
};

#endif // AXONBRANCH_H

