#include "Position.h"
#include "vtkincludes.h"

Position::Position(double x, double y, double z) : x(x), y(y), z(z) {}

// Function to calculate the Euclidean distance between two positions
[[nodiscard]] double Position::distanceTo(const Position &other) const {
    double diff[3] = {x - other.x, y - other.y, z - other.z};
    return vtkMath::Norm(diff);
}

// Function to set the position coordinates
void Position::set(double newX, double newY, double newZ) {
    x = newX;
    y = newY;
    z = newZ;
}

double Position::calcPropagationTime(const Position &position1, double propagationRate) const {
    double distance = this->distanceTo(position1);
    return distance / propagationRate;
}

bool Position::operator==(const Position &other) const {
    return (x == other.x) && (y == other.y) && (z == other.z);
}

std::tuple<double, double, double> Position::getPosition() const {
    std::cout << "Position: " << x << ", " << y << ", " << z << std::endl;
    return std::make_tuple(x, y, z);
}
