#ifndef AXONHILLOCK_H
#define AXONHILLOCK_H

#include <memory>
#include <vector>
#include "NeuronalComponent.h"
#include "Position.h"

// Forward declarations to avoid circular dependencies
class Soma;
class Axon;

class AxonHillock : public NeuronalComponent
{
public:
    explicit AxonHillock(const std::shared_ptr<Position>& position, std::weak_ptr<NeuronalComponent> parent);

    ~AxonHillock() override = default;

    // Override methods from NeuronalComponent
    void initialise() override;
    void update(double deltaTime);

    // AxonHillock-specific methods
    [[nodiscard]] std::shared_ptr<Axon> getAxon() const;
    void updateFromSoma(std::shared_ptr<Soma> parentPointer);
    [[nodiscard]] std::shared_ptr<Soma> getParentSoma() const;
    void setAxonHillockId(int id);
    int getAxonHillockId() const;

private:
    // Member variables
    std::shared_ptr<Axon> onwardAxon;
    std::shared_ptr<Soma> parentSoma;
    int axonHillockId = -1;
};

#endif // AXONHILLOCK_H
