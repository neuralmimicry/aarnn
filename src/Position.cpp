#include <iostream>
#include <mutex>
#include <string_view>
#include <syncstream>
#include <thread>


#include "../include/Position.h"
#include "../include/vtkincludes.h"

Position::Position(double x, double y, double z) : x(x), y(y), z(z) {}

    double x, y, z;

    // Function to calculate the Euclidean distance between two positions
    [[nodiscard]] double Position::distanceTo(Position& other) {
        double diff[3] = { x - other.x, y - other.y, z - other.z };
        return vtkMath::Norm(diff);
    }

    // Function to set the position coordinates
    void Position::set(double newX, double newY, double newZ) {
        x = newX;
        y = newY;
        z = newZ;
    }

    double Position::calcPropagationTime(Position& position1, double propagationRate) {
        double distance = 0;
        distance = this->distanceTo(position1);
        return distance / propagationRate;
    }

