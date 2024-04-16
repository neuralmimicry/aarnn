#ifndef POSITION_H_INCLUDED
#define POSITION_H_INCLUDED

#include <array>

class Position
{
    private:
    public:
    Position(double x, double y, double z);
    double x;
    double y;
    double z;
    
    void   set(double newX, double newY, double newZ);
    double calcPropagationTime(Position& position1, double propagationRate);

    // Comparison operator for Position class
    bool operator==(const Position& other) const;

    [[nodiscard]] double distanceTo(Position& other);

    [[nodiscard]] std::array<double, 3> get() const
    {
        return {x, y, z};
    }
};

#endif  // POSITION_H_INCLUDED
