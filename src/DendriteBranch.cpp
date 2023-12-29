#include <iostream>
#include "../include/DendriteBranch.h"


// Dendrite branch class

    explicit DendriteBranch::DendriteBranch(const PositionPtr& position) : BodyComponent(position), BodyShapeComponent() {
        // On construction set a default propagation rate
        propagationRate = 0.5;
    }
    ~DendriteBranch::DendriteBranch() override = default;

    void DendriteBranch::initialise() {
        if (!instanceInitialised) {
            if (onwardDendrites.empty()) {
                connectDendrite(std::make_shared<Dendrite>(std::make_shared<Position>((position->x) - 1, (position->y) - 1, (position->z) - 1)));
                onwardDendrites.back()->initialise();
                onwardDendrites.back()->updateFromDendriteBranch(shared_from_this());
            }
            instanceInitialised = true;
        }
    }

    void DendriteBranch::updatePosition(const PositionPtr& newPosition) {
        position = newPosition;
    }


    void DendriteBranch::connectDendrite(std::shared_ptr<Dendrite> dendrite) {
        auto coords = get_coordinates(int (onwardDendrites.size() + 1), int(onwardDendrites.size() + 1), int(5));
        PositionPtr currentPosition = dendrite->getPosition();
        auto x = double(std::get<0>(coords)) + currentPosition->x;
        auto y = double(std::get<1>(coords)) + currentPosition->y;
        auto z = double(std::get<2>(coords)) + currentPosition->z;
        currentPosition->x = x;
        currentPosition->y = y;
        currentPosition->z = z;
        onwardDendrites.emplace_back(std::move(dendrite));
    }

    void DendriteBranch::updateFromSoma(std::shared_ptr<Soma> parentSomaPointer) { parentSoma = std::move(parentSomaPointer); }


    void DendriteBranch::updateFromDendrite(std::shared_ptr<Dendrite> parentDendritePointer) { parentDendrite = std::move(parentDendritePointer); }