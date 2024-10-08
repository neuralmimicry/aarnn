#ifndef POSITION_H
#define POSITION_H

#include <array>

class Position
{
public:
    Position(double x, double y, double z);

    /**
     * @brief Function to get the x-coordinate.
     * @return The x-coordinate.
     */
    double getX() const;

    /**
     * @brief Function to get the y-coordinate.
     * @return The y-coordinate.
     */
    double getY() const;

    /**
     * @brief Function to get the z-coordinate.
     * @return The z-coordinate.
     */
    double getZ() const;

    /**
     * @brief Function to calculate the Euclidean distance between two positions.
     * @param other The other position.
     * @return The Euclidean distance between the two positions.
     */
    [[nodiscard]] double distanceTo(Position &other);

    /**
     * @brief Function to set the position coordinates.
     * @param newX The new x-coordinate.
     * @param newY The new y-coordinate.
     * @param newZ The new z-coordinate.
     */
    void set(double newX, double newY, double newZ);

    /**
     * @brief Function to calculate the propagation time between two positions.
     * @param position1 The other position.
     * @param propagationRate The propagation rate.
     * @return The propagation time between the two positions.
     */
    double calcPropagationTime(Position &position1, double propagationRate);

    /**
     * @brief Function to get the position coordinates.
     * @return An array containing the x, y, and z coordinates.
     */
    [[nodiscard]] std::array<double, 3> get() const;

    /**
     * @brief Comparison operator for Position class.
     * @param other The other position.
     * @return True if the two positions are equal, false otherwise.
     */
    bool operator==(const Position &other) const;

// private:
    /**
     * @brief The x-coordinate.
     */
    double x;

    /**
     * @brief The y-coordinate.
     */
    double y;

    /**
     * @brief The z-coordinate.
     */
    double z;
};

#endif // POSITION_H
