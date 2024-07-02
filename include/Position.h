#ifndef POSITION_H
#define POSITION_H

#include <tuple>
#include <iostream>

class Position {
public:
    double x, y, z;

    Position(double x, double y, double z);

    [[nodiscard]] double distanceTo(const Position &other) const;
    void set(double newX, double newY, double newZ);
    double calcPropagationTime(const Position &position1, double propagationRate) const;
    bool operator==(const Position &other) const;
    std::tuple<double, double, double> getPosition() const;
};

#endif // POSITION_H
