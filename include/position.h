#ifndef POSITION_H_INCLUDED
#define POSITION_H_INCLUDED

// #define DO_TRACE_
#include "traceutil.h"

#include <cmath>
#include <exception>
#include <spdlog/spdlog.h>
#include <sstream>
#include <string>
#include <tuple>

namespace ns_aarnn
{
struct position_error : public std::invalid_argument
{
    explicit position_error(const std::string& function, const std::string& message)
    : std::invalid_argument(function + ": " + message)
    {
        spdlog::error(message);
    }
};

/**
 * @brief class to hold a position of an object in a 3 dimensional space.
 *
 */
class Position
{
    public:
    static constexpr double BODY_DISTANCE_DEFAULT         = 0.1;
    static constexpr double BODY_DISTANCE_DEFAULT_SQUARED = BODY_DISTANCE_DEFAULT * BODY_DISTANCE_DEFAULT;

    explicit Position(double x = 0.0, double y = 0.0, double z = 0.0) : x_(x), y_(y), z_(z)
    {
    }

    /**
     * @brief get the x-component.
     *
     * @return double the x-component
     */
    [[nodiscard]] double x() const
    {
        return x_;
    }

    /**
     * @brief get the y-component.
     *
     * @return double the y-component
     */
    [[nodiscard]] double y() const
    {
        return y_;
    }

    /**
     * @brief get the z-component.
     *
     * @return double the z-component
     */
    [[nodiscard]] double z() const
    {
        return z_;
    }

    /**
     * Translate position by offset.
     *
     * @param offset the offset to move by
     * @return the new position of this object
     */
    Position move(const Position& offset)
    {
        x_ += offset.x_;
        y_ += offset.y_;
        z_ += offset.z_;
        return *this;
    }

    /**
     * Translate position by coordinates.
     *
     * @param x offset in x-direction
     * @param y offset in y-direction
     * @param z offset in z-direction
     * @return the new position of this object
     */
    Position move(double x, double y, double z)
    {
        x_ += x;
        y_ += y;
        z_ += z;
        return *this;
    }

    /**
     * Move this point to a position relative to a given position.
     * @param otherPosition the position relative to which the new position is defined.
     * @param x offset in x-direction from otherPosition
     * @param y offset in y-direction from otherPosition
     * @param z offset in yy-direction from otherPosition
     * @return the new position of this object
     */
    Position moveRelativeTo(const Position& otherPosition, double x, double y, double z)
    {
        return move(x + otherPosition.x(), y + otherPosition.y(), z + otherPosition.z());
    }

    /**
     * Scale the position relative to the origin (0,0,0) by a factor.
     * @param factor the factor to scale by
     * @return the new position of this object
     */
    Position scale(double factor)
    {
        x_ *= factor;
        y_ *= factor;
        z_ *= factor;
        return *this;
    }

    /**
     * @brief Equality (component-wise) of two positions.
     * @param lhs left hand side position
     * @param rhs right hand side position
     * @return true, if all coordinates are equal, false otherwise
     */
    friend bool operator==(const Position& lhs, const Position& rhs)
    {
        return fabs(lhs.x_ - rhs.x_) + fabs(lhs.y_ - rhs.y_) + fabs(lhs.z_ - rhs.z_) < 1e-12;
    }

    /**
     * @brief Inequality (component-wise) of two positions.
     * @param lhs left hand side position
     * @param rhs right hand side position
     * @return true, if any of the coordinates are not equal, false otherwise
     */
    friend bool operator!=(const Position& lhs, const Position& rhs)
    {
        return !(lhs == rhs);
    }

    /**
     * @brief Calculate the Euclidean distance between two positions.
     *
     * @param other the other position
     * @return double
     */
    [[nodiscard]] double distanceTo(const Position& other) const
    {
        return std::sqrt((x_ - other.x_) * (x_ - other.x_) + (y_ - other.y_) * (y_ - other.y_)
                         + (z_ - other.z_) * (z_ - other.z_));
    }

    /**
     * @brief std out-stream operator.
     *
     * @param os stream to modify
     * @param position position to serialise
     * @return std::ostream& the modified out stream
     */
    friend std::ostream& operator<<(std::ostream& os, const Position& position)
    {
        os << "pos[" << position.x() << "," << position.y() << "," << position.z() << "]";
        return os;
    }

    //    template<typename IntT_>
    //    static Position layeredFibonacciSpherePoint(IntT_ i_pointIndex, IntT_ i_totalPoints, IntT_ i_pointsPerLayer)
    //    {
    //        if(i_pointIndex < static_cast<IntT_>(0))
    //        {
    //            throw position_error(__PRETTY_FUNCTION__, "Negative point index not allowed");
    //        }
    //
    //        if(i_totalPoints <= static_cast<IntT_>(0))
    //        {
    //            throw position_error(__PRETTY_FUNCTION__, "Number of total points must be greater than zero (0)");
    //        }
    //
    //        if(i_pointsPerLayer <= static_cast<IntT_>(0))
    //        {
    //            throw position_error(__PRETTY_FUNCTION__,
    //                                         "Number of points per layer must be greater than zero (0)");
    //        }
    //        static const double GOLDEN_ANGLE = M_PI * (3.0 - std::sqrt(5.0));  // golden angle in radians
    //
    //        auto pointIndex     = static_cast<double>(i_pointIndex);
    //        auto totalPoints    = static_cast<double>(i_totalPoints);
    //        auto numberOfLayers = i_totalPoints / i_pointsPerLayer + 1;
    //        TRACE3(pointIndex, totalPoints, numberOfLayers)
    //        auto layerOfPoint = i_pointIndex / numberOfLayers;
    //        auto layerRadius  = (1.0 / (static_cast<double>(numberOfLayers + 1))) * (layerOfPoint + 1);
    //        auto y            = 1.0 - (pointIndex / (totalPoints - 1.0)) * 2.0;
    //        TRACE3(layerOfPoint, layerRadius, y)
    //        auto radius = std::sqrt(1.0 - y * y);     // y can assume values from -1.0 to 1.0
    //        auto theta  = GOLDEN_ANGLE * pointIndex;  // golden angle increment
    //        auto x      = std::cos(theta) * radius;
    //        auto z      = std::sin(theta) * radius;
    //        TRACE3(x, y, z)
    //        auto reval = Position{x * layerRadius, y * layerRadius, z * layerRadius};
    //        TRACE1(reval)
    //        return reval;
    //    }

    /**
     * @brief Generate coordinates for a point on one of the spheres of layered concentric Fibonacci-spheres.
     * The layer is calculated in a manner that ensures that all the points on the each layer have the same distance to
     * the next layer.
     * @tparam IntT_ integral type
     * @param i_pointIndex index of the point
     * @param i_totalPoints total number of points
     * @return coordinates for point i_pointIndex as Position object
     */
    template<typename IntT_>
    static Position layeredFibonacciSpherePoint(IntT_ i_pointIndex, IntT_ i_totalPoints)
    {
        // Validate inputs
        if(i_pointIndex < 0 || i_totalPoints <= 0)
        {
            throw position_error(
             __PRETTY_FUNCTION__,
             "Index and total points must be non-negative and total points must be greater than zero.");
        }

        // Calculate the number of spheres needed
        double sphereRadius          = 0.0;
        double pointsOnCurrentSphere = 0;
        int    sphereIndex           = 0;

        while(pointsOnCurrentSphere < i_totalPoints)
        {
            sphereIndex++;
            sphereRadius = sphereIndex * BODY_DISTANCE_DEFAULT;
            pointsOnCurrentSphere += 4 * M_PI * std::pow(sphereRadius, 2) / BODY_DISTANCE_DEFAULT_SQUARED;
        }

        // Calculate the position of the point using the Fibonacci sphere algorithm
        double phi    = M_PI * (3.0 - std::sqrt(5.0));                           // golden angle in radians
        double y      = 1.0 - (i_pointIndex / double(i_totalPoints - 1)) * 2.0;  // y is interval [1..-1]
        double radius = std::sqrt(1.0 - y * y);                                  // radius at y

        double theta = phi * i_pointIndex;  // golden angle increment

        double x = std::cos(theta) * radius;
        double z = std::sin(theta) * radius;

        // Scale the position to the correct sphere
        double scale = sphereRadius / std::sqrt(x * x + y * y + z * z);

        auto reval = Position{x * scale, y * scale, z * scale};
        TRACE1(reval)
        return reval;
    }

    private:
    double x_{};
    double y_{};
    double z_{};
};

}  // namespace aarnn

#endif  // POSITION_H_INCLUDED
