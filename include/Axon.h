// Axon.h
#ifndef AXON_H
#define AXON_H

#include <memory>
#include <vector>
#include "Position.h"
#include "AxonBouton.h"
#include "AxonBranch.h"
#include "AxonHillock.h"
#include "BodyComponent.h"
#include "BodyShapeComponent.h"

class Axon : public std::enable_shared_from_this<Axon>, public BodyComponent<Position>, public BodyShapeComponent {
public:
    explicit Axon(const PositionPtr& position);
    ~Axon() override = default;
    void initialise();
    void updatePosition(const PositionPtr& newPosition);
    void addBranch(std::shared_ptr<AxonBranch> branch);
    [[nodiscard]] const std::vector<std::shared_ptr<AxonBranch>>& getAxonBranches() const;
    [[nodiscard]] std::shared_ptr<AxonBouton> getAxonBouton() const;
    double calcPropagationTime();
    void updateFromAxonHillock(std::shared_ptr<AxonHillock> parentAxonHillockPointer);
    [[nodiscard]] std::shared_ptr<AxonHillock> getParentAxonHillock() const;
    void updateFromAxonBranch(std::shared_ptr<AxonBranch> parentAxonBranchPointer);
    [[nodiscard]] std::shared_ptr<AxonBranch> getParentAxonBranch() const;
    [[nodiscard]] const PositionPtr& getPosition() const;
private:
    bool instanceInitialised = false;
    std::vector<std::shared_ptr<AxonBranch>> axonBranches;
    std::shared_ptr<AxonBouton> onwardAxonBouton;
    std::shared_ptr<AxonHillock> parentAxonHillock;
    std::shared_ptr<AxonBranch> parentAxonBranch;
};

#endif // AXON_H
