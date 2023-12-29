#ifndef AXONBRANCH_H
#define AXONBRANCH_H

#include <memory>
#include <vector>

#include "Position.h"
#include "BodyComponent.h"
#include "BodyShapeComponent.h"
#include "Axon.h"

class AxonBranch : public std::enable_shared_from_this<AxonBranch>, public BodyComponent<Position>, public BodyShapeComponent {
public:
    explicit AxonBranch(const PositionPtr& position);
    ~AxonBranch() override = default;
    void initialise() ;
    void updatePosition(const PositionPtr& newPosition) ;
    void connectAxon(std::shared_ptr<Axon> axon) ;
    [[nodiscard]] const std::vector<std::shared_ptr<Axon>>& getAxons() const {return onwardAxons;}
    void updateFromAxon(std::shared_ptr<Axon> parentPointer);
    [[nodiscard]] std::shared_ptr<Axon> getParentAxon() const { return parentAxon; }
    [[nodiscard]] const PositionPtr& getPosition() const { return position; }
private:
    bool instanceInitialised = false;
    std::vector<std::shared_ptr<Axon>> onwardAxons;
    std::shared_ptr<Axon> parentAxon;
};

#endif