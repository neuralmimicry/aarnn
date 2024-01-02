#include "../include/position.h"
#include <vtkMath.h>

Position::Position(double x, double y, double z) : x(x), y(y), z(z)
{
}

double Position::getX() const
{
    return x;
}

double Position::getY() const
{
    return y;
}

double Position::getZ() const
{
    return z;
}

double Position::distanceTo(Position &other)
{
    double diff[3] = {getX() - other.getX(), getY() - other.getY(), getZ() - other.getZ()};
    return vtkMath::Norm(diff);
}

void Position::set(double newX, double newY, double newZ)
{
    x = newX;
    y = newY;
    z = newZ;
}

double Position::calcPropagationTime(Position &position1, double propagationRate)
{
    double distance = 0;
    distance = distanceTo(position1);
    return distance / propagationRate;
}

std::array<double, 3> Position::get() const
{
    return {x, y, z};
}

