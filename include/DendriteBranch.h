#ifndef DENDRITEBRANCH_H
#define DENDRITEBRANCH_H

#include <memory>
#include "BodyComponent.h"
#include "BodyShapeComponent.h"
#include "Position.h"
#include "SynapticGap.h"
#include "Dendrite.h"
#include "Soma.h"

class Soma;
class Dendrite;

class DendriteBranch : public std::enable_shared_from_this<DendriteBranch>, public BodyComponent<Position>, public BodyShapeComponent {
public:
    explicit DendriteBranch(const PositionPtr& position) : BodyComponent(position), BodyShapeComponent() {
        // On construction set a default propagation rate
        propagationRate = 0.5;
    }
    ~DendriteBranch() override = default;
    void initialise() ;
    void updatePosition(const PositionPtr& newPosition) ;
    void connectDendrite(std::shared_ptr<Dendrite> dendrite);
    [[nodiscard]] std::vector<std::shared_ptr<Dendrite>> getDendrites() const { return onwardDendrites; }
    void updateFromSoma(std::shared_ptr<Soma> parentSomaPointer) ;
    [[nodiscard]] std::shared_ptr<Soma> getParentSoma() const { return parentSoma; }
    void updateFromDendrite(std::shared_ptr<Dendrite> parentDendritePointer) ;
    [[nodiscard]] std::shared_ptr<Dendrite> getParentDendrite() const { return parentDendrite; }
    [[nodiscard]] const PositionPtr& getPosition() const { return position; }
private:
    bool instanceInitialised = false;  // Initially, the Dendrite is not initialised
    std::vector<std::shared_ptr<Dendrite>> onwardDendrites;
    std::shared_ptr<Soma> parentSoma;
    std::shared_ptr<Dendrite> parentDendrite;
};

#endif