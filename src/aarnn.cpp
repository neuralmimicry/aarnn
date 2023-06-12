/**
 * @file  aarnn.cpp
 * @brief This file contains the implementation of a neuronal network simulation.
 * @author Paul B. Isaac's
 * @version 0.1
 * @date  12-May-2023
 */
#include <iostream>
#include <utility>
#include <vector>
#include <list>
#include <algorithm>
#include <thread>
#include <mutex>
#include <atomic>
#include <cmath>
#include <memory>
#include <pqxx/pqxx>
#include <stdexcept>
#include <fstream>
#include <string>
#include <tuple>
#include <map>
#include <set>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <random>

#include "boostincludes.h"

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

extern "C" {
#include <libavcodec/avcodec.h>
}

#include "vtkincludes.h"

#include "aarnn.h"
#include "dendrite.h"
#include "dendritebranch.h"

std::mt19937 gen(12345); // Always generates the same sequence of numbers
std::uniform_real_distribution<> distr(-0.15, 1.0-0.15);

// websocketpp typedefs
typedef websocketpp::server<websocketpp::config::asio> Server;
typedef websocketpp::connection_hdl ConnectionHandle;

/*
Setup VTK environment? - note, not required in core aarnn file, only in the visualiser
 */

/**
 * @brief Read configuration files and return a map of key-value pairs.
 * @param filenames Vector of filenames to read the configuration from.
 * @return A map containing the key-value pairs from the configuration files.
 */
 std::map<std::string, std::string> read_config(const std::vector<std::string> &filenames) {
    std::map<std::string, std::string> config;

    for (const auto &filename : filenames) {
        std::ifstream file(filename);
        std::string line;

        while (std::getline(file, line)) {
            std::string key, value;
            std::istringstream line_stream(line);
            std::getline(line_stream, key, '=');
            std::getline(line_stream, value);
            config[key] = value;
        }
    }

    return config;
}

std::tuple<double, double, double> get_coordinates(int i, int total_points, int points_per_layer) {
    const double golden_angle = M_PI * (3 - std::sqrt(5)); // golden angle in radians

    // calculate the layer and index in layer based on i
    int layer = i / points_per_layer;
    int index_in_layer = i % points_per_layer;

    // normalize radial distance based on layer number
    double r = (double)(layer + 1) / (double(total_points) / points_per_layer);

    // azimuthal angle based on golden angle
    double theta = golden_angle * index_in_layer;

    // height y is a function of layer
    double y = 1 - (double)(layer) / (double(total_points) / points_per_layer - 1);

    // radius at y
    double radius = std::sqrt(1 - y*y);

    // polar angle evenly distributed
    double phi = 2 * M_PI * (double)(index_in_layer) / points_per_layer;

    // Convert spherical coordinates to Cartesian coordinates
    double x = r * radius * std::cos(theta);
    y = r * y;
    double z = r * radius * std::sin(theta);

    return std::make_tuple(x, y, z);
}

/**
 * @brief Build a connection string for the database using the configuration map.
 * @param config A map containing the key-value pairs from the configuration files.
 * @return A connection string containing the configuration for the database.
 */
std::string build_connection_string(const std::map<std::string, std::string> &config) {
    std::string connection_string;

    // List of allowed connection parameters
    std::set<std::string> allowed_params = {"host", "port", "user", "password", "dbname"};

    for (const auto &entry : config) {
        if (allowed_params.count(entry.first) > 0) {
            connection_string += entry.first + "=" + entry.second + " ";
        }
    }

    return connection_string;
}

// Logger class to write log messages to a file
class Logger {
public:
    /**
     * @brief Construct a new Logger object.
     * @param filename The name of the file to write log messages to.
     */

    explicit Logger(const std::string &filename) : log_file(filename, std::ofstream::out | std::ofstream::app) {}

    /**
     * @brief Destructor. Closes the log file.
     */
    ~Logger() {
        log_file.close();
    }

    /**
     * @brief Stream insertion operator to write messages to the log file.
     * @tparam T Type of the message.
     * @param msg The message to log.
     * @return Reference to the Logger object.
     */
    template<typename T>
    Logger &operator<<(const T &msg) {
        log_file << msg;
        log_file.flush();
        return *this;
    }

    /**
     * @brief Stream insertion operator overload for std::endl.
     * @param pf Pointer to the std::endl function.
     * @return Reference to the Logger object.
     */
    Logger &operator<<(std::ostream& (*pf)(std::ostream&)) {
        log_file << pf;
        log_file.flush();
        return *this;
    }

private:
    /**
     * @brief Output file stream to write log messages.
     */
    std::ofstream log_file;
};

class Shape3D {
public:
    Shape3D() = default;
    ~Shape3D() = default;

    // Add methods and data members to represent the 3D shape, e.g., vertices, faces, etc.
    // Use VTK smart pointers for shape representation if needed
};

// Add a new class to store neuron parameters and access the database
class NeuronParameters {
public:
    NeuronParameters() {
        // Access the database and load neuron parameters
    }

    /**
    * @brief Getter function for dendrite propagation rate.
    * @return The dendrite propagation rate.
    */
    // double getDendritePropagationRate() const;

private:
    /**
     * @brief Neuron parameter: dendrite propagation rate.
     */
    // double dendritePropagationRate;
};

/**
 * @class Position
 * @brief Class representing (x, y, z) coordinates in 3D space.
 */
class Position {
public:
    Position(double x, double y, double z) : x(x), y(y), z(z) {}

    double x, y, z;

    // Function to calculate the Euclidean distance between two positions
    [[nodiscard]] double distanceTo(Position& other) {
        double diff[3] = { x - other.x, y - other.y, z - other.z };
        return vtkMath::Norm(diff);
    }

    // Function to set the position coordinates
    void set(double newX, double newY, double newZ) {
        x = newX;
        y = newY;
        z = newZ;
    }

    double calcPropagationTime(Position& position1, double propagationRate) {
        double distance = 0;
        distance = this->distanceTo(position1);
        return distance / propagationRate;
    }

    [[nodiscard]] std::array<double, 3> get() const {
        return {x, y, z};
    }

    // Comparison operator for Position class
    bool operator==(const Position& other) const {
        return (this == &other);
    }

};

using PositionPtr = std::shared_ptr<Position>;

// Neuron component base class
template <typename PositionType>
class BodyComponent {
public:
    using PositionPtr = std::shared_ptr<PositionType>;

    explicit BodyComponent(PositionPtr  position) : position(std::move(position)) {}
    virtual ~BodyComponent() = default;

    [[nodiscard]] virtual const PositionPtr& getPosition() const {
        return position;
    }

    virtual void receiveStimulation(int8_t stimulation) {
        // Implement the stimulation logic
        propagationRate += double((propagationRate * 0.01) * stimulation);
        // Clamp propagationRate within the range 0.1 to 0.9
        propagationRate = propagationRate < 0.1 || propagationRate > 0.9 ? 0 : propagationRate;
    }

    virtual double getPropagationRate() {
        // Implement the calculation based on the synapse properties
        return propagationRate;
    }

protected:
    PositionPtr position;
    double propagationRate{0.5};
};

// Neuron shape component base class
class BodyShapeComponent {
public:
    using ShapePtr = std::shared_ptr<Shape3D>;

    BodyShapeComponent() = default;
    virtual ~BodyShapeComponent() = default;

    virtual void setShape(const ShapePtr& newShape) {
        this->shape = newShape;
    }

    [[nodiscard]] virtual const ShapePtr& getShape() const {
        return shape;
    }

private:
    ShapePtr shape;
};

/// Synaptic gap class
class SynapticGap : public std::enable_shared_from_this<SynapticGap>, public BodyComponent<Position>, public BodyShapeComponent {
public:
    explicit SynapticGap(const PositionPtr& position) : BodyComponent(position), BodyShapeComponent() {
        // On construction set a default propagation rate
        propagationRate = 0.5;
    }
    ~SynapticGap() override = default;

    void initialise() {
        if (!instanceInitialised) {
            instanceInitialised = true;
        }
    }

    void updatePosition(const PositionPtr& newPosition) {
        position = newPosition;
    }


    // Method to check if the SynapticGap has been associated
    bool isAssociated() const {
        return associated;
    }
    // Method to set the SynapticGap as associated
    void setAsAssociated() {
        associated = true;
    }

    void updateFromSensoryReceptor(std::shared_ptr<SensoryReceptor> parentSensoryReceptorPointer) { parentSensoryReceptor = std::move(parentSensoryReceptorPointer); }

    [[nodiscard]] std::shared_ptr<SensoryReceptor> getParentSensoryReceptor() const { return parentSensoryReceptor; }

    void updateFromEffector(std::shared_ptr<Effector>& parentEffectorPointer) { parentEffector = std::move(parentEffectorPointer); }

    [[nodiscard]] std::shared_ptr<Effector> getParentEffector() const { return parentEffector; }

    void updateFromAxonBouton(std::shared_ptr<AxonBouton> parentAxonBoutonPointer) { parentAxonBouton = std::move(parentAxonBoutonPointer); }

    [[nodiscard]] std::shared_ptr<AxonBouton> getParentAxonBouton() const { return parentAxonBouton; }

    void updateFromDendriteBouton(std::shared_ptr<DendriteBouton> parentDendriteBoutonPointer) { parentDendriteBouton = std::move(parentDendriteBoutonPointer); }

    [[nodiscard]] std::shared_ptr<DendriteBouton> getParentDendriteBouton() const { return parentDendriteBouton; }

    void updateComponent(double time, double energy) {
        componentEnergyLevel = calculateEnergy(time, componentEnergyLevel + energy);// Update the component
        // for (auto& synapticGap_id : synapticGaps) {
        //     synapticGap_id->updateComponent(time + propagationTime(), componentEnergyLevel);
        // }
    }

    double calculateEnergy(double currentTime, double currentEnergyLevel) {
        double deltaTime = currentTime - previousTime;
        previousTime = currentTime;
        energyLevel = currentEnergyLevel;

        if (deltaTime < attack) {
            return (deltaTime / attack) * calculateWaveform(currentTime);
        } else if (deltaTime < attack + decay) {
            double decay_time = deltaTime - attack;
            return ((1 - decay_time / decay) * (1 - sustain) + sustain) * calculateWaveform(currentTime);
        } else if (deltaTime < attack + decay + sustain) {
            return sustain * calculateWaveform(currentTime);
        } else {
            double release_time = deltaTime - attack - decay - sustain;
            return (1 - std::max(0.0, release_time / release)) * calculateWaveform(currentTime);
        }
    }

    double calculateWaveform(double currentTime) const {
        return energyLevel * sin(2 * M_PI * frequencyResponse * currentTime + phaseShift);
    }

    double propagationTime() {
        double currentTime = (double) std::clock() / CLOCKS_PER_SEC;
        callCount++;
        double timeSinceLastCall = currentTime - lastCallTime;
        lastCallTime = currentTime;

        double x = 1 / (1 + exp(-callCount / timeSinceLastCall));
        return minPropagationTime + x * (maxPropagationTime - minPropagationTime);
    }

    [[nodiscard]] const PositionPtr& getPosition() const {
        return position;
    }

private:
    bool instanceInitialised = false;  // Initially, the SynapticGap is not initialised
    bool associated = false;  // Initially, the SynapticGap is not associated with a DendriteBouton
    std::shared_ptr<Effector> parentEffector;
    std::shared_ptr<SensoryReceptor> parentSensoryReceptor;
    std::shared_ptr<AxonBouton> parentAxonBouton;
    std::shared_ptr<DendriteBouton> parentDendriteBouton;
    double attack;
    double decay;
    double sustain;
    double release;
    double frequencyResponse;
    double phaseShift;
    double previousTime;
    double energyLevel;
    double componentEnergyLevel;
    double minPropagationTime;
    double maxPropagationTime;
    double lastCallTime;
    int callCount;
};


// Dendrite Bouton class
class DendriteBouton : public std::enable_shared_from_this<DendriteBouton>, public BodyComponent<Position>, public BodyShapeComponent {
public:
    explicit DendriteBouton(const PositionPtr& position) : BodyComponent(position), BodyShapeComponent() {
        // On construction set a default propagation rate
        propagationRate = 0.5;
    }
    ~DendriteBouton() override = default;

    void initialise() {
        if (!instanceInitialised) {
            instanceInitialised = true;
        }
    }

    void connectSynapticGap(std::shared_ptr<SynapticGap> gap);
    void updatePosition(const PositionPtr& newPosition) {
        position = newPosition;
    }


    [[nodiscard]] std::shared_ptr<SynapticGap> getSynapticGap() const {
        return onwardSynapticGap;
    }

    double getPropagationRate() override {
        // Implement the calculation based on the dendrite bouton properties
        return propagationRate;
    }

    void setNeuron(std::weak_ptr<Neuron> parentNeuron) {
        this->neuron = std::move(parentNeuron);
    }

    void updateFromDendrite(std::shared_ptr<Dendrite> parentDendritePointer) { parentDendrite = std::move(parentDendritePointer); }

    [[nodiscard]] std::shared_ptr<Dendrite> getParentDendrite() const { return parentDendrite; }

    [[nodiscard]] const PositionPtr& getPosition() const {
        return position;
    }

private:
    bool instanceInitialised = false;  // Initially, the DendriteBouton is not initialised
    std::shared_ptr<SynapticGap> onwardSynapticGap;
    std::weak_ptr<Neuron> neuron;
    std::shared_ptr<Dendrite> parentDendrite;
};

// Dendrite class
class Dendrite : public std::enable_shared_from_this<Dendrite>, public BodyComponent<Position>, public BodyShapeComponent {
public:
    explicit Dendrite(const PositionPtr& position) : BodyComponent(position), BodyShapeComponent() {
        // On construction set a default propagation rate
        propagationRate = 0.5;
    }
    ~Dendrite() override = default;

    void initialise() {
        if (!instanceInitialised) {
            if (!dendriteBouton) {
                dendriteBouton = std::make_shared<DendriteBouton>(std::make_shared<Position>((position->x) - 1, (position->y) - 1, (position->z) - 1));
                dendriteBouton->initialise();
                dendriteBouton->updateFromDendrite(shared_from_this());
            }
            instanceInitialised = true;
        }
    }

    void updatePosition(const PositionPtr& newPosition) {
        position = newPosition;
    }


    void addBranch(std::shared_ptr<DendriteBranch> branch);

    [[nodiscard]] const std::vector<std::shared_ptr<DendriteBranch>>& getDendriteBranches() const {
        return dendriteBranches;
    }

    [[nodiscard]] const std::shared_ptr<DendriteBouton>& getDendriteBouton() const {
        return dendriteBouton;
    }

    void updateFromDendriteBranch(std::shared_ptr<DendriteBranch> parentDendriteBranchPointer) { parentDendriteBranch = std::move(parentDendriteBranchPointer); }

    [[nodiscard]] std::shared_ptr<DendriteBranch> getParentDendriteBranch() const { return parentDendriteBranch; }

    [[nodiscard]] const PositionPtr& getPosition() const {
        return position;
    }

private:
    bool instanceInitialised = false;  // Initially, the Dendrite is not initialised
    std::vector<std::shared_ptr<DendriteBranch>> dendriteBranches;
    std::shared_ptr<DendriteBranch> dendriteBranch;
    std::shared_ptr<DendriteBouton> dendriteBouton;
    std::shared_ptr<DendriteBranch> parentDendriteBranch;
};

// Dendrite branch class
class DendriteBranch : public std::enable_shared_from_this<DendriteBranch>, public BodyComponent<Position>, public BodyShapeComponent {
public:
    explicit DendriteBranch(const PositionPtr& position) : BodyComponent(position), BodyShapeComponent() {
        // On construction set a default propagation rate
        propagationRate = 0.5;
    }
    ~DendriteBranch() override = default;

    void initialise() {
        if (!instanceInitialised) {
            if (onwardDendrites.empty()) {
                connectDendrite(std::make_shared<Dendrite>(std::make_shared<Position>((position->x) - 1, (position->y) - 1, (position->z) - 1)));
                onwardDendrites.back()->initialise();
                onwardDendrites.back()->updateFromDendriteBranch(shared_from_this());
            }
            instanceInitialised = true;
        }
    }

    void updatePosition(const PositionPtr& newPosition) {
        position = newPosition;
    }


    void connectDendrite(std::shared_ptr<Dendrite> dendrite) {
        auto coords = get_coordinates(int (onwardDendrites.size() + 1), int(onwardDendrites.size() + 1), int(5));
        PositionPtr currentPosition = dendrite->getPosition();
        double x = std::get<0>(coords) + currentPosition->x;
        double y = std::get<1>(coords) + currentPosition->y;
        double z = std::get<2>(coords) + currentPosition->z;
        currentPosition->x = x;
        currentPosition->y = y;
        currentPosition->z = z;
        onwardDendrites.emplace_back(std::move(dendrite));
    }

    [[nodiscard]] std::vector<std::shared_ptr<Dendrite>> getDendrites() const {
        return onwardDendrites;
    }

    void updateFromSoma(std::shared_ptr<Soma> parentSomaPointer) { parentSoma = std::move(parentSomaPointer); }

    [[nodiscard]] std::shared_ptr<Soma> getParentSoma() const { return parentSoma; }

    void updateFromDendrite(std::shared_ptr<Dendrite> parentDendritePointer) { parentDendrite = std::move(parentDendritePointer); }

    [[nodiscard]] std::shared_ptr<Dendrite> getParentDendrite() const { return parentDendrite; }

    [[nodiscard]] const PositionPtr& getPosition() const {
        return position;
    }

private:
    bool instanceInitialised = false;  // Initially, the Dendrite is not initialised
    std::vector<std::shared_ptr<Dendrite>> onwardDendrites;
    std::shared_ptr<Soma> parentSoma;
    std::shared_ptr<Dendrite> parentDendrite;
};

void Dendrite::addBranch(std::shared_ptr<DendriteBranch> branch) {
    auto coords = get_coordinates(int (dendriteBranches.size() + 1), int(dendriteBranches.size() + 1), int(5));
    PositionPtr currentPosition = branch->getPosition();
    double x = std::get<0>(coords) + currentPosition->x;
    double y = std::get<1>(coords) + currentPosition->y;
    double z = std::get<2>(coords) + currentPosition->z;
    currentPosition->x = x;
    currentPosition->y = y;
    currentPosition->z = z;
    dendriteBranches.emplace_back(std::move(branch));
}

// Axon bouton class
class AxonBouton : public std::enable_shared_from_this<AxonBouton>, public BodyComponent<Position>, public BodyShapeComponent {
public:
    explicit AxonBouton(const PositionPtr& position) : BodyComponent(position),
                                                           BodyShapeComponent() {
    }
    ~AxonBouton() override = default;

    void initialise() {
        if (!instanceInitialised) {
            if (!onwardSynapticGap) {
                onwardSynapticGap = std::make_shared<SynapticGap>(std::make_shared<Position>((position->x) + 1, (position->y) + 1, (position->z) + 1));
            }
            onwardSynapticGap->initialise();
            onwardSynapticGap->updateFromAxonBouton(shared_from_this());
            instanceInitialised = true;
        }
    }

    void updatePosition(const PositionPtr& newPosition) {
        position = newPosition;
    }


    void AddSynapticGap(const std::shared_ptr<SynapticGap>& gap) {
        gap->updateFromAxonBouton(shared_from_this());
    }

    void connectSynapticGap(std::shared_ptr<SynapticGap> gap);

    [[nodiscard]] std::shared_ptr<SynapticGap> getSynapticGap() const {
        return onwardSynapticGap;
    }

    void setNeuron(std::weak_ptr<Neuron> parentNeuron) {
        this->neuron = std::move(parentNeuron);
    }

    void updateFromAxon(std::shared_ptr<Axon> parentAxonPointer) { parentAxon = std::move(parentAxonPointer); }

    [[nodiscard]] std::shared_ptr<Axon> getParentAxon() const { return parentAxon; }

    [[nodiscard]] const PositionPtr& getPosition() const {
        return position;
    }

private:
    bool instanceInitialised = false;
    std::shared_ptr<SynapticGap> onwardSynapticGap;
    std::weak_ptr<Neuron> neuron;
    std::shared_ptr<Axon> parentAxon;
};

// Axon class
class Axon : public std::enable_shared_from_this<Axon>, public BodyComponent<Position>, public BodyShapeComponent {
public:
    explicit Axon(const PositionPtr& position) : BodyComponent(position), BodyShapeComponent() {
        // On construction set a default propagation rate
        propagationRate = 0.5;
    }
    ~Axon() override = default;

    void initialise() {
        if (!instanceInitialised) {
            if (!onwardAxonBouton) {
                onwardAxonBouton = std::make_shared<AxonBouton>(std::make_shared<Position>((position->x) + 1, (position->y) + 1, (position->z) + 1));
            }
            onwardAxonBouton->initialise();
            onwardAxonBouton->updateFromAxon(shared_from_this());
            instanceInitialised = true;
        }
    }

    void updatePosition(const PositionPtr& newPosition) {
        position = newPosition;
    }


    void addBranch(std::shared_ptr<AxonBranch> branch);

    [[nodiscard]] const std::vector<std::shared_ptr<AxonBranch>>& getAxonBranches() const {
        return axonBranches;
    }

    [[nodiscard]] std::shared_ptr<AxonBouton> getAxonBouton() const {
        return onwardAxonBouton;
    }

    double calcPropagationTime();

    void updateFromAxonHillock(std::shared_ptr<AxonHillock> parentAxonHillockPointer) { parentAxonHillock = std::move(parentAxonHillockPointer); }

    [[nodiscard]] std::shared_ptr<AxonHillock> getParentAxonHillock() const { return parentAxonHillock; }

    void updateFromAxonBranch(std::shared_ptr<AxonBranch> parentAxonBranchPointer) { parentAxonBranch = std::move(parentAxonBranchPointer); }

    [[nodiscard]] std::shared_ptr<AxonBranch> getParentAxonBranch() const { return parentAxonBranch; }

    [[nodiscard]] const PositionPtr& getPosition() const {
        return position;
    }

private:
    bool instanceInitialised = false;
    std::vector<std::shared_ptr<AxonBranch>> axonBranches;
    std::shared_ptr<AxonBouton> onwardAxonBouton;
    std::shared_ptr<AxonHillock> parentAxonHillock;
    std::shared_ptr<AxonBranch> parentAxonBranch;
};

// Axon branch class
class AxonBranch : public std::enable_shared_from_this<AxonBranch>, public BodyComponent<Position>, public BodyShapeComponent {
public:
    explicit AxonBranch(const PositionPtr& position) : BodyComponent(position), BodyShapeComponent() {
        // On construction set a default propagation rate
        propagationRate = 0.5;
    }
    ~AxonBranch() override = default;

    void initialise() {
        if (!instanceInitialised) {
            if (onwardAxons.empty()) {
                connectAxon(std::make_shared<Axon>(std::make_shared<Position>((position->x) + 1, (position->y) + 1, (position->z) + 1)));
                onwardAxons.back()->initialise();
                onwardAxons.back()->updateFromAxonBranch(shared_from_this());
            }
            instanceInitialised = true;
        }
    }

    void updatePosition(const PositionPtr& newPosition) {
        position = newPosition;
    }


    void connectAxon(std::shared_ptr<Axon> axon) {
        auto coords = get_coordinates(int (onwardAxons.size() + 1), int(onwardAxons.size() + 1), int(5));
        PositionPtr currentPosition = axon->getPosition();
        double x = std::get<0>(coords) + currentPosition->x;
        double y = std::get<1>(coords) + currentPosition->y;
        double z = std::get<2>(coords) + currentPosition->z;
        currentPosition->x = x;
        currentPosition->y = y;
        currentPosition->z = z;
        onwardAxons.emplace_back(std::move(axon));
    }

    [[nodiscard]] const std::vector<std::shared_ptr<Axon>>& getAxons() const {
        return onwardAxons;
    }
    void updateFromAxon(std::shared_ptr<Axon> parentPointer) { parentAxon = std::move(parentPointer); }

    [[nodiscard]] std::shared_ptr<Axon> getParentAxon() const { return parentAxon; }

    [[nodiscard]] const PositionPtr& getPosition() const {
        return position;
    }

private:
    bool instanceInitialised = false;
    std::vector<std::shared_ptr<Axon>> onwardAxons;
    std::shared_ptr<Axon> parentAxon;
};

void Axon::addBranch(std::shared_ptr<AxonBranch> branch) {
    auto coords = get_coordinates(int (axonBranches.size() + 1), int(axonBranches.size() + 1), int(5));
    PositionPtr currentPosition = branch->getPosition();
    double x = std::get<0>(coords) + currentPosition->x;
    double y = std::get<1>(coords) + currentPosition->y;
    double z = std::get<2>(coords) + currentPosition->z;
    currentPosition->x = x;
    currentPosition->y = y;
    currentPosition->z = z;
    axonBranches.emplace_back(std::move(branch));
}


// Axon hillock class
class AxonHillock : public std::enable_shared_from_this<AxonHillock>, public BodyComponent<Position>, public BodyShapeComponent {
public:
    explicit AxonHillock(const PositionPtr& position) : BodyComponent(position), BodyShapeComponent() {
        // On construction set a default propagation rate
        propagationRate = 0.5;
    }
    ~AxonHillock() override = default;

    void initialise() {
        if (!instanceInitialised) {
            if (!onwardAxon) {
                onwardAxon = std::make_shared<Axon>(std::make_shared<Position>((position->x) + 1, (position->y) + 1, (position->z) + 1));
            }
            onwardAxon->initialise();
            onwardAxon->updateFromAxonHillock(shared_from_this());
            instanceInitialised = true;
        }
    }

    void updatePosition(const PositionPtr& newPosition) {
        position = newPosition;
    }


    [[nodiscard]] std::shared_ptr<Axon> getAxon() const {
        return onwardAxon;
    }

    void updateFromSoma(std::shared_ptr<Soma> parentPointer) { parentSoma = std::move(parentPointer); }

    [[nodiscard]] std::shared_ptr<Soma> getParentSoma() const { return parentSoma; }

    [[nodiscard]] const PositionPtr& getPosition() const {
        return position;
    }

private:
    bool instanceInitialised = false;
    std::shared_ptr<Axon> onwardAxon;
    std::shared_ptr<Soma> parentSoma;
};

double Axon::calcPropagationTime() {
    double distance = 0;
    PositionPtr positionPointer1;
    PositionPtr positionPointer2;
    PositionPtr positionPointerCurrent = this->getPosition();

    if (!positionPointerCurrent) {
        std::cerr << "Error: Current position pointer is null." << std::endl;
        return 0;
    }

    if (parentAxonHillock) {
        positionPointer1 = parentAxonHillock->getPosition();
    } else if (parentAxonBranch) {
        positionPointer1 = parentAxonBranch->getPosition();
    } else {
        std::cerr << "Error: Both parentAxonHillock and parentAxonBranch are null." << std::endl;
        return 0;
    }

    positionPointer2 = onwardAxonBouton->getPosition();

    if (!positionPointer1 || !positionPointer2) {
        std::cerr << "Error: Either positionPointer1 or positionPointer2 is null." << std::endl;
        return 0;
    }

    distance = positionPointer1->distanceTo(*positionPointerCurrent) + positionPointer2->distanceTo(*positionPointerCurrent);
    return distance / propagationRate;
}


class Soma : public std::enable_shared_from_this<Soma>, public BodyComponent<Position>, public BodyShapeComponent {
public:
    explicit Soma(const PositionPtr& position) : BodyComponent(position), BodyShapeComponent() {
        // On construction set a default propagation rate
        propagationRate = 0.5;
    }
    ~Soma() override = default;

    void initialise() {
        if (!instanceInitialised) {
            if (!onwardAxonHillock) {
                onwardAxonHillock = std::make_shared<AxonHillock>(std::make_shared<Position>((position->x) + 1, (position->y) + 1, (position->z) + 1));
            }
            onwardAxonHillock->initialise();
            onwardAxonHillock->updateFromSoma(shared_from_this());
            addDendriteBranch(std::make_shared<DendriteBranch>(std::make_shared<Position>((position->x) - 1, (position->y) - 1, (position->z) - 1)));
            dendriteBranches.back()->initialise();
            dendriteBranches.back()->updateFromSoma(shared_from_this());
            instanceInitialised = true;
        }
    }

    void updatePosition(const PositionPtr& newPosition) {
        position = newPosition;
    }


    [[nodiscard]] std::shared_ptr<AxonHillock> getAxonHillock() const {
        return onwardAxonHillock;
    }

    void addDendriteBranch(std::shared_ptr<DendriteBranch> dendriteBranch) {
        auto coords = get_coordinates(int (dendriteBranches.size() + 1), int(dendriteBranches.size() + 1), int(5));
        PositionPtr currentPosition = dendriteBranch->getPosition();
        double x = std::get<0>(coords) + currentPosition->x;
        double y = std::get<1>(coords) + currentPosition->y;
        double z = std::get<2>(coords) + currentPosition->z;
        currentPosition->x = x;
        currentPosition->y = y;
        currentPosition->z = z;
        dendriteBranches.emplace_back(std::move(dendriteBranch));
    }

    [[nodiscard]] const std::vector<std::shared_ptr<DendriteBranch>>& getDendriteBranches() const {
        return dendriteBranches;
    }

    void updateFromNeuron(std::shared_ptr<Neuron> parentPointer) { parentNeuron = std::move(parentPointer); }

    [[nodiscard]] std::shared_ptr<Neuron> getParentNeuron() const { return parentNeuron; }

    [[nodiscard]] const PositionPtr& getPosition() const {
        return position;
    }

private:
    bool instanceInitialised = false;
    std::vector<std::shared_ptr<SynapticGap>> synapticGaps;
    std::vector<std::shared_ptr<DendriteBranch>> dendriteBranches;
    std::shared_ptr<AxonHillock> onwardAxonHillock;
    std::shared_ptr<Neuron> parentNeuron;
};

// Sensory Actor class
class Effector : public std::enable_shared_from_this<Effector>, public BodyComponent<Position>, public BodyShapeComponent {
public:
    explicit Effector(const PositionPtr& position) : BodyComponent(position), BodyShapeComponent() {
        // On construction set a default propagation rate
        propagationRate = 0.5;
    }
    ~Effector() override = default;

    void initialise() {
        if (!instanceInitialised) {
            instanceInitialised = true;
        }
    }

    void addSynapticGap(std::shared_ptr<SynapticGap> synapticGap) {
        synapticGaps.emplace_back(std::move(synapticGap));
    }

    void updatePosition(const PositionPtr& newPosition) {
        position = newPosition;
    }

    [[nodiscard]] std::vector<std::shared_ptr<SynapticGap>> getSynapticGaps() const {
        return synapticGaps;
    }

    double getPropagationRate() override {
        // Implement the calculation based on the dendrite bouton properties
        return propagationRate;
    }

    [[nodiscard]] const PositionPtr& getPosition() const {
        return position;
    }

private:
    bool instanceInitialised = false;  // Initially, the DendriteBouton is not initialised
    std::vector<std::shared_ptr<SynapticGap>> synapticGaps;
};

// Sensory Receptor class
class SensoryReceptor : public std::enable_shared_from_this<SensoryReceptor>, public BodyComponent<Position>, public BodyShapeComponent {
public:
    explicit SensoryReceptor(const PositionPtr& position) : BodyComponent(position),
                                                       BodyShapeComponent() {
    }
    ~SensoryReceptor() override = default;

    void initialise() {
        if (!instanceInitialised) {
            setAttack((35 - (rand() % 25)) / 100);
            setDecay((35 - (rand() % 25)) / 100);
            setSustain((35 - (rand() % 25)) / 100);
            setRelease((35 - (rand() % 25)) / 100);
            setFrequencyResponse(rand() % 44100);
            setPhaseShift(rand() % 360);
            lastCallTime = 0.0;
            synapticGap = std::make_shared<SynapticGap>(
                    std::make_shared<Position>((position->x) + 1, (position->y) + 1, (position->z) + 1));
            synapticGap->initialise();
            synapticGap->updateFromSensoryReceptor(shared_from_this());
            synapticGaps.emplace_back(std::move(synapticGap));
            minPropagationRate = (35 - (rand() % 25)) / 100;
            maxPropagationRate = (65 + (rand() % 25)) / 100;
            instanceInitialised = true;
        }
    }

    void addSynapticGap(std::shared_ptr<SynapticGap> gap) {
        synapticGaps.emplace_back(std::move(gap));
    }

    void updatePosition(const PositionPtr& newPosition) {
        position = newPosition;
    }

    [[nodiscard]] std::vector<std::shared_ptr<SynapticGap>> getSynapticGaps() const {
        return synapticGaps;
    }

    void setAttack(double newAttack) {
        this->attack = newAttack;
    }

    void setDecay(double newDecay) {
        this->decay = newDecay;
    }

    void setSustain(double newSustain) {
        this->sustain = newSustain;
    }

    void setRelease(double newRelease) {
        this->release = newRelease;
    }

    void setFrequencyResponse(double newFrequencyResponse) {
        this->frequencyResponse = newFrequencyResponse;
    }

    void setPhaseShift(double newPhaseShift) {
        this->phaseShift = newPhaseShift;
    }

    void setEnergyLevel(double newEnergyLevel) {
        this->energyLevel = newEnergyLevel;
    }

    double calculateEnergy(double currentTime, double currentEnergyLevel) {
        double deltaTime = currentTime - previousTime;
        previousTime = currentTime;
        energyLevel = currentEnergyLevel;

        if (deltaTime < attack) {
            return (deltaTime / attack) * calculateWaveform(currentTime);
        } else if (deltaTime < attack + decay) {
            double decay_time = deltaTime - attack;
            return ((1 - decay_time / decay) * (1 - sustain) + sustain) * calculateWaveform(currentTime);
        } else if (deltaTime < attack + decay + sustain) {
            return sustain * calculateWaveform(currentTime);
        } else {
            double release_time = deltaTime - attack - decay - sustain;
            return (1 - std::max(0.0, release_time / release)) * calculateWaveform(currentTime);
        }
    }

    double calculateWaveform(double currentTime) const {
        return energyLevel * sin(2 * M_PI * frequencyResponse * currentTime + phaseShift);
    }

    double calcPropagationRate() {
        double currentTime = (double) std::clock() / CLOCKS_PER_SEC;
        callCount++;
        double timeSinceLastCall = currentTime - lastCallTime;
        lastCallTime = currentTime;

        double x = 1 / (1 + exp(-callCount / timeSinceLastCall));
        return minPropagationRate + x * (maxPropagationRate - minPropagationRate);
    }

    void updateComponent(double time, double energy) {
        componentEnergyLevel = calculateEnergy(time, componentEnergyLevel + energy);// Update the component
        propagationRate = calcPropagationRate();
        for (auto& synapticGap_id : synapticGaps) {
            synapticGap_id->updateComponent(time + position->calcPropagationTime(*synapticGap_id->getPosition(), propagationRate), componentEnergyLevel);
        }
        componentEnergyLevel = 0;
    }

    [[nodiscard]] const PositionPtr& getPosition() const {
        return position;
    }

private:
    bool instanceInitialised = false;
    std::vector<std::shared_ptr<SynapticGap>> synapticGaps;
    std::shared_ptr<SynapticGap> synapticGap;
    double attack;
    double decay;
    double sustain;
    double release;
    double frequencyResponse;
    double phaseShift;
    double previousTime;
    double energyLevel;
    double componentEnergyLevel;
    double minPropagationRate;
    double maxPropagationRate;
    double lastCallTime;
    int callCount;
};

/**
 * @brief Neuron class representing a neuron in the simulation.
 */
class Neuron : public std::enable_shared_from_this<Neuron>, public BodyComponent<Position>, public BodyShapeComponent {
public:
    explicit Neuron(const PositionPtr& position) : BodyComponent(position), BodyShapeComponent() {
        /**
         * @brief Construct a new Neuron object.
         * @param soma Shared pointer to the Soma object of the neuron.
         */
    }
    /**
     * @brief Destroy the Neuron object.
     */
    ~Neuron() override= default;

    // Alternative to constructor so that weak pointers can be established
    void initialise() {
        if (!instanceInitialised) {
            if (!soma) {
                soma = std::make_shared<Soma>(std::make_shared<Position>(position->x, position->y, position->z));
            }
            soma->initialise();
            soma->updateFromNeuron(shared_from_this());
            instanceInitialised = true;
        }
    }

    /**
     * @brief Getter function for the soma of the neuron.
     * @return Shared pointer to the Soma object of the neuron.
     */
    std::shared_ptr<Soma> getSoma() {
        return soma;
    }

    void updatePosition(const PositionPtr& newPosition) {
        position = newPosition;
    }

    void storeAllSynapticGapsAxon() {
        // Clear the synapticGaps vector before traversing
        synapticGapsAxon.clear();
        axonBoutons.clear();

        // Get the onward axon hillock from the soma
        std::shared_ptr<AxonHillock> onwardAxonHillock = soma->getAxonHillock();
        if (!onwardAxonHillock) {
            return;
        }

        // Get the onward axon from the axon hillock
        std::shared_ptr<Axon> onwardAxon = onwardAxonHillock->getAxon();
        if (!onwardAxon) {
            return;
        }

        // Recursively traverse the onward axons and their axon boutons to store all synaptic gaps
        traverseAxonsForStorage(onwardAxon);
    }

    void storeAllSynapticGapsDendrite() {
        // Clear the synapticGaps vector before traversing
        synapticGapsDendrite.clear();
        dendriteBoutons.clear();
        
        // Recursively traverse the onward dendrite branches, dendrites and their dendrite boutons to store all synaptic gaps
        traverseDendritesForStorage(soma->getDendriteBranches());
    }
    
    void addSynapticGapAxon(std::shared_ptr<SynapticGap> synapticGap) {
        synapticGapsAxon.emplace_back(std::move(synapticGap));
    }

    [[nodiscard]] const std::vector<std::shared_ptr<SynapticGap>>& getSynapticGapsAxon() const {
        return synapticGapsAxon;
    }

    void addSynapticGapDendrite(std::shared_ptr<SynapticGap> synapticGap) {
        synapticGapsDendrite.emplace_back(std::move(synapticGap));
    }

    [[nodiscard]] const std::vector<std::shared_ptr<SynapticGap>>& getSynapticGapsDendrite() const {
        return synapticGapsDendrite;
    }

    void addDendriteBouton(std::shared_ptr<DendriteBouton> dendriteBouton) {
        dendriteBoutons.emplace_back(std::move(dendriteBouton));
    }

    [[nodiscard]] const std::vector<std::shared_ptr<DendriteBouton>>& getDendriteBoutons() const {
        return dendriteBoutons;
    }

    [[nodiscard]] const PositionPtr& getPosition() const {
        return position;
    }

private:
    bool instanceInitialised = false;
    std::shared_ptr<Soma> soma;
    std::vector<std::shared_ptr<SynapticGap>> synapticGapsAxon;
    std::vector<std::shared_ptr<SynapticGap>> synapticGapsDendrite;
    std::vector<std::shared_ptr<AxonBouton>> axonBoutons;
    std::vector<std::shared_ptr<DendriteBouton>> dendriteBoutons;

    // Helper function to recursively traverse the axons and find the synaptic gap
    std::shared_ptr<SynapticGap> traverseAxons(const std::shared_ptr<Axon>& axon, const PositionPtr& positionPtr) {
        // Check if the current axon's bouton has a gap using the same position pointer
        std::shared_ptr<AxonBouton> axonBouton = axon->getAxonBouton();
        if (axonBouton) {
            std::shared_ptr<SynapticGap> gap = axonBouton->getSynapticGap();
            if (gap->getPosition() == positionPtr) {
                return gap;
            }
        }

        // Recursively traverse the axon's branches and onward axons
        const std::vector<std::shared_ptr<AxonBranch>>& branches = axon->getAxonBranches();
        for (const auto& branch : branches) {
            const std::vector<std::shared_ptr<Axon>>& onwardAxons = branch->getAxons();
            for (const auto& onwardAxon : onwardAxons) {
                std::shared_ptr<SynapticGap> gap = traverseAxons(onwardAxon, positionPtr);
                if (gap) {
                    return gap;
                }
            }
        }

        return nullptr;
    }

    // Helper function to recursively traverse the dendrites and find the synaptic gap
    std::shared_ptr<SynapticGap> traverseDendrites(const std::shared_ptr<Dendrite>& dendrite, const PositionPtr& positionPtr) {
        // Check if the current dendrite's bouton is near the same position pointer
        std::shared_ptr<DendriteBouton> dendriteBouton = dendrite->getDendriteBouton();
        if (dendriteBouton) {
            std::shared_ptr<SynapticGap> gap = dendriteBouton->getSynapticGap();
            if (gap->getPosition() == positionPtr) {
                return gap;
            }
        }

        // Recursively traverse the dendrite's branches and onward dendrites
        const std::vector<std::shared_ptr<DendriteBranch>>& dendriteBranches = dendrite->getDendriteBranches();
        for (const auto& dendriteBranch : dendriteBranches) {
            const std::vector<std::shared_ptr<Dendrite>>& childDendrites = dendriteBranch->getDendrites();
            for (const auto& onwardDendrites : childDendrites) {
                std::shared_ptr<SynapticGap> gap = traverseDendrites(onwardDendrites, positionPtr);
                if (gap) {
                    return gap;
                }
            }
        }

        return nullptr;
    }

    // Helper function to recursively traverse the axons and store all synaptic gaps
    void traverseAxonsForStorage(const std::shared_ptr<Axon>& axon) {
        // Check if the current axon's bouton has a gap and store it
        std::shared_ptr<AxonBouton> axonBouton = axon->getAxonBouton();
        if (axonBouton) {
            std::shared_ptr<SynapticGap> gap = axonBouton->getSynapticGap();
            synapticGapsAxon.emplace_back(std::move(gap));
        }

        // Recursively traverse the axon's branches and onward axons
        const std::vector<std::shared_ptr<AxonBranch>>& branches = axon->getAxonBranches();
        for (const auto& branch : branches) {
            const std::vector<std::shared_ptr<Axon>>& onwardAxons = branch->getAxons();
            for (const auto& onwardAxon : onwardAxons) {
                traverseAxonsForStorage(onwardAxon);
            }
        }
    }

    // Helper function to recursively traverse the dendrites and store all dendrite boutons
    void traverseDendritesForStorage(const std::vector<std::shared_ptr<DendriteBranch>>& dendriteBranches) {
        for (const auto& branch : dendriteBranches) {
            const std::vector<std::shared_ptr<Dendrite>>& onwardDendrites = branch->getDendrites();
            for (const auto& onwardDendrite : onwardDendrites) {
                // Check if the current dendrite's bouton has a gap and store it
                std::shared_ptr<DendriteBouton> dendriteBouton = onwardDendrite->getDendriteBouton();
                std::shared_ptr<SynapticGap> gap = dendriteBouton->getSynapticGap();
                if (gap) {
                    synapticGapsDendrite.emplace_back(std::move(gap));
                }
                traverseDendritesForStorage(onwardDendrite->getDendriteBranches());
            }
        }
    }
};

void AxonBouton::connectSynapticGap(std::shared_ptr<SynapticGap> gap)
{
    onwardSynapticGap = std::move(gap);
    if (auto spt = neuron.lock()) { // has to check if the shared_ptr is still valid
        spt->addSynapticGapAxon(onwardSynapticGap);
    }
}

void DendriteBouton::connectSynapticGap(std::shared_ptr<SynapticGap> gap)
{
    onwardSynapticGap = std::move(gap);
    if (auto spt = neuron.lock()) { // has to check if the shared_ptr is still valid
        spt->addSynapticGapDendrite(onwardSynapticGap);
    }
}

// Global variables
std::atomic<double> totalPropagationRate(0.0);
std::mutex mtx;

/**
 * @brief Compute the propagation rate of a neuron.
 * @param neuron Pointer to the Neuron object for which the propagation rate is to be calculated.
 */
void computePropagationRate(const std::shared_ptr<Neuron>& neuron) {
    // Get the propagation rate of the neuron from the DendriteBouton to the SynapticGap
    double propagationRate = neuron->getSoma()->getPropagationRate();

    // Lock the mutex to safely update the totalPropagationRate
    {
        std::lock_guard<std::mutex> lock(mtx);
        totalPropagationRate = totalPropagationRate + propagationRate;
    }
}

void associateSynapticGap(Neuron& neuron1, Neuron& neuron2, double proximityThreshold) {
    // Iterate over all SynapticGaps in neuron1
    for (auto& gap : neuron1.getSynapticGapsAxon()) {
        // If the bouton has a synaptic gap, and it is not associated yet
        if (gap && !gap->isAssociated()) {
            // Iterate over all DendriteBoutons in neuron2
            for (auto& dendriteBouton : neuron2.getDendriteBoutons()) {
                // If the distance between the bouton and the dendriteBouton is below the proximity threshold
                if (gap->getPosition()->distanceTo(*(dendriteBouton->getPosition())) < proximityThreshold) {
                    // Associate the synaptic gap with the dendriteBouton
                    dendriteBouton->connectSynapticGap(gap);
                    // Set the SynapticGap as associated
                    gap->setAsAssociated();
                    // No need to check other DendriteBoutons once the gap is associated
                    break;
                }
            }
        }
    }
}

void associateSynapticGap(SensoryReceptor& receptor, Neuron& neuron, double proximityThreshold) {
    // Iterate over all SynapticGaps of receptor
    for (auto& gap : receptor.getSynapticGaps()) {
        // If the bouton has a synaptic gap, and it is not associated yet
        if (gap && !gap->isAssociated()) {
            // Iterate over all DendriteBoutons in neuron
            for (auto& dendriteBouton : neuron.getDendriteBoutons()) {
                // If the distance between the gap and the dendriteBouton is below the proximity threshold
                if (gap->getPosition()->distanceTo(*(dendriteBouton->getPosition())) < proximityThreshold) {
                    // Associate the synaptic gap with the dendriteBouton
                    dendriteBouton->connectSynapticGap(gap);
                    // Set the SynapticGap as associated
                    gap->setAsAssociated();
                    // No need to check other DendriteBoutons once the gap is associated
                    break;
                }
            }
        }
    }
}

/**
 * @brief Initialize the database, checking and creating the required table if needed.
 * @param conn Reference to the pqxx::connection object.
 */
void initialize_database(pqxx::connection& conn) {
    pqxx::work txn(conn);

    // Check if the "neurons" table exists
    txn.exec(
            "DROP TABLE IF EXISTS neurons, somas, axonhillocks, axons, axonboutons, synapticgaps, dendritebranches, dendrites, dendriteboutons CASCADE; COMMIT;"
    );

    // Check if the "neurons" table exists
    pqxx::result result = txn.exec(
            "SELECT EXISTS ("
            "SELECT FROM pg_tables WHERE schemaname = 'public' AND tablename = 'neurons'"
            ");"
    );

    if (!result.empty() && !result[0][0].as<bool>()) {
        // Create the "neurons" table if it does not exist
        txn.exec(
                "CREATE TABLE neurons ("
                "neuron_id SERIAL PRIMARY KEY,"
                "x REAL NOT NULL,"
                "y REAL NOT NULL,"
                "z REAL NOT NULL,"
                "propagation_rate REAL,"
                "neuron_type INTEGER,"
                "axon_length REAL"
                ");"

                "CREATE TABLE somas ("
                "soma_id SERIAL PRIMARY KEY,"
                "neuron_id INTEGER REFERENCES neurons(neuron_id),"
                "x REAL NOT NULL,"
                "y REAL NOT NULL,"
                "z REAL NOT NULL"
                ");"

                "CREATE TABLE axonhillocks ("
                "axon_hillock_id SERIAL PRIMARY KEY,"
                "soma_id INTEGER REFERENCES somas(soma_id),"
                "x REAL NOT NULL,"
                "y REAL NOT NULL,"
                "z REAL NOT NULL"
                ");"

                "CREATE TABLE axons ("
                "axon_id SERIAL PRIMARY KEY,"
                "axon_hillock_id INTEGER REFERENCES axonhillocks(axon_hillock_id),"
                "x REAL NOT NULL,"
                "y REAL NOT NULL,"
                "z REAL NOT NULL"
                ");"

                "CREATE TABLE axonboutons ("
                "axon_bouton_id SERIAL PRIMARY KEY,"
                "axon_id INTEGER REFERENCES axons(axon_id),"
                "x REAL NOT NULL,"
                "y REAL NOT NULL,"
                "z REAL NOT NULL"
                ");"

                "CREATE TABLE synapticgaps ("
                "synaptic_gap_id SERIAL PRIMARY KEY,"
                "axon_bouton_id INTEGER REFERENCES axonboutons(axon_bouton_id),"
                "x REAL NOT NULL,"
                "y REAL NOT NULL,"
                "z REAL NOT NULL"
                ");"

                "CREATE TABLE dendritebranches ("
                "dendrite_branch_id SERIAL PRIMARY KEY,"
                "soma_id INTEGER REFERENCES somas(soma_id),"
                "x REAL NOT NULL,"
                "y REAL NOT NULL,"
                "z REAL NOT NULL"
                ");"

                "CREATE TABLE dendrites ("
                "dendrite_id SERIAL PRIMARY KEY,"
                "dendrite_branch_id INTEGER REFERENCES dendritebranches(dendrite_branch_id),"
                "x REAL NOT NULL,"
                "y REAL NOT NULL,"
                "z REAL NOT NULL"
                ");"

                "CREATE TABLE dendriteboutons ("
                "dendrite_bouton_id SERIAL PRIMARY KEY,"
                "dendrite_id INTEGER REFERENCES dendrites(dendrite_id),"
                "x REAL NOT NULL,"
                "y REAL NOT NULL,"
                "z REAL NOT NULL"
                ");"
        );

        txn.commit();
        std::cout << "Created table 'neurons'." << std::endl;
    } else {
        std::cout << "Table 'neurons' already exists." << std::endl;
    }
}

void addDendriteBranchToRenderer(vtkRenderer* renderer, const std::shared_ptr<DendriteBranch>& dendriteBranch,
                                 const std::shared_ptr<Dendrite>& dendrite,
                                 vtkSmartPointer<vtkPoints> points, vtkSmartPointer<vtkIdList> pointIds) {
    const PositionPtr& dendriteBranchPosition = dendriteBranch->getPosition();
    const PositionPtr& dendritePosition = dendrite->getPosition();

    // Calculate the radii at each end
    double radius1 = 0.5;  // Radius at the start point (dendrite branch)
    double radius2 = 0.25;  // Radius at the end point (dendrite)

    // Compute the vector from the start point to the end point
    double vectorX = dendritePosition->x - dendriteBranchPosition->x;
    double vectorY = dendritePosition->y - dendriteBranchPosition->y;
    double vectorZ = dendritePosition->z - dendriteBranchPosition->z;

    // Compute the length of the vector
    double length = std::sqrt(vectorX * vectorX + vectorY * vectorY + vectorZ * vectorZ);

    // Compute the midpoint of the vector
    double midpointX = (dendritePosition->x + dendriteBranchPosition->x) / 2.0;
    double midpointY = (dendritePosition->y + dendriteBranchPosition->y) / 2.0;
    double midpointZ = (dendritePosition->z + dendriteBranchPosition->z) / 2.0;

    // Create the cone with the desired dimensions
    vtkSmartPointer<vtkConeSource> coneSource = vtkSmartPointer<vtkConeSource>::New();
    coneSource->SetRadius(radius1);
    coneSource->SetHeight(length);
    coneSource->SetResolution(50);  // Set resolution for a smooth cone

    vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
    transform->Scale(radius2 / radius1, radius2 / radius1, 1.0);

    vtkSmartPointer<vtkTransformPolyDataFilter> transformFilter = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
    transformFilter->SetInputConnection(coneSource->GetOutputPort());
    transformFilter->SetTransform(transform);
    transformFilter->Update();

    vtkSmartPointer<vtkPolyDataMapper> coneMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    coneMapper->SetInputData(transformFilter->GetOutput());

    vtkSmartPointer<vtkActor> coneActor = vtkSmartPointer<vtkActor>::New();
    coneActor->SetMapper(coneMapper);
    coneActor->GetProperty()->SetColor(0.0, 1.0, 0.0);  // Set colour to green

    // Translate the cone to the midpoint position
    vtkSmartPointer<vtkTransform> translationTransform = vtkSmartPointer<vtkTransform>::New();
    translationTransform->Translate(midpointX, midpointY, midpointZ);
    coneActor->SetUserTransform(translationTransform);

    // Add the actor to the renderer
    renderer->AddActor(coneActor);
}

void addAxonBranchToRenderer(vtkRenderer* renderer, const std::shared_ptr<AxonBranch>& axonBranch,
                             const std::shared_ptr<Axon>& axon,
                             vtkSmartPointer<vtkPoints> points, vtkSmartPointer<vtkIdList> pointIds) {
    const PositionPtr& axonBranchPosition = axonBranch->getPosition();
    const PositionPtr& axonPosition = axon->getPosition();

    // Calculate the radii at each end
    double radius1 = 0.5;  // Radius at the start point (dendrite branch)
    double radius2 = 0.25;  // Radius at the end point (dendrite)

    // Compute the vector from the start point to the end point
    double vectorX = axonPosition->x - axonBranchPosition->x;
    double vectorY = axonPosition->y - axonBranchPosition->y;
    double vectorZ = axonPosition->z - axonBranchPosition->z;

    // Compute the length of the vector
    double length = std::sqrt(vectorX * vectorX + vectorY * vectorY + vectorZ * vectorZ);

    // Compute the midpoint of the vector
    double midpointX = (axonPosition->x + axonBranchPosition->x) / 2.0;
    double midpointY = (axonPosition->y + axonBranchPosition->y) / 2.0;
    double midpointZ = (axonPosition->z + axonBranchPosition->z) / 2.0;

    // Create the cone with the desired dimensions
    vtkSmartPointer<vtkConeSource> coneSource = vtkSmartPointer<vtkConeSource>::New();
    coneSource->SetRadius(radius1);
    coneSource->SetHeight(length);
    coneSource->SetResolution(50);  // Set resolution for a smooth cone

    vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
    transform->Scale(radius2 / radius1, radius2 / radius1, 1.0);

    vtkSmartPointer<vtkTransformPolyDataFilter> transformFilter = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
    transformFilter->SetInputConnection(coneSource->GetOutputPort());
    transformFilter->SetTransform(transform);
    transformFilter->Update();

    vtkSmartPointer<vtkPolyDataMapper> coneMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    coneMapper->SetInputData(transformFilter->GetOutput());

    vtkSmartPointer<vtkActor> coneActor = vtkSmartPointer<vtkActor>::New();
    coneActor->SetMapper(coneMapper);
    coneActor->GetProperty()->SetColor(1.0, 0.0, 0.0);  // Set colour to red

    // Translate the cone to the midpoint position
    vtkSmartPointer<vtkTransform> translationTransform = vtkSmartPointer<vtkTransform>::New();
    translationTransform->Translate(midpointX, midpointY, midpointZ);
    coneActor->SetUserTransform(translationTransform);

    // Add the actor to the renderer
    renderer->AddActor(coneActor);
}

void addSynapticGapToRenderer(vtkRenderer* renderer, const std::shared_ptr<SynapticGap>& synapticGap) {
    const PositionPtr& synapticGapPosition = synapticGap->getPosition();

    // Create the sphere source for the glyph
    vtkSmartPointer<vtkSphereSource> sphereSource = vtkSmartPointer<vtkSphereSource>::New();
    sphereSource->SetRadius(0.175);  // Set the radius of the sphere

    // Create a vtkPoints object to hold the positions of the spheres
    vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();

    // Add multiple points around the synaptic gap position to create a cloud
    for (int i = 0; i < 8; ++i) {
        double offsetX = distr(gen);
        double offsetY = distr(gen);
        double offsetZ = distr(gen);

        double x = synapticGapPosition->x + offsetX;
        double y = synapticGapPosition->y + offsetY;
        double z = synapticGapPosition->z + offsetZ;

        points->InsertNextPoint(x, y, z);
    }

    // Create a vtkPolyData object and set the points as its vertices
    vtkSmartPointer<vtkPolyData> polyData = vtkSmartPointer<vtkPolyData>::New();
    polyData->SetPoints(points);

    // Create the glyph filter and set the polydata as the input
    vtkSmartPointer<vtkGlyph3D> glyph3D = vtkSmartPointer<vtkGlyph3D>::New();
    glyph3D->SetInputData(polyData);
    glyph3D->SetSourceConnection(sphereSource->GetOutputPort());
    glyph3D->SetScaleFactor(1.0);  // Set the scale factor for the glyph

    // Create a mapper and actor for the glyph
    vtkSmartPointer<vtkPolyDataMapper> glyphMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    glyphMapper->SetInputConnection(glyph3D->GetOutputPort());

    vtkSmartPointer<vtkActor> glyphActor = vtkSmartPointer<vtkActor>::New();
    glyphActor->SetMapper(glyphMapper);
    glyphActor->GetProperty()->SetColor(1.0, 1.0, 0.0);  // Set color to yellow

    // Add the glyph actor to the renderer
    renderer->AddActor(glyphActor);
}


void addDendriteBranchPositionsToPolyData(vtkRenderer* renderer, const std::shared_ptr<DendriteBranch>& dendriteBranch, const vtkSmartPointer<vtkPoints>& definePoints, vtkSmartPointer<vtkIdList> pointIDs) {
    // Add the position of the dendrite branch to vtkPolyData
    PositionPtr branchPosition = dendriteBranch->getPosition();
    pointIDs->InsertNextId(definePoints->InsertNextPoint(branchPosition->x, branchPosition->y, branchPosition->z));

    // Iterate over the dendrites and add their positions to vtkPolyData
    const std::vector<std::shared_ptr<Dendrite>>& dendrites = dendriteBranch->getDendrites();
    for (const auto& dendrite : dendrites) {
        PositionPtr dendritePosition = dendrite->getPosition();
        addDendriteBranchToRenderer(renderer, dendriteBranch, dendrite, definePoints, pointIDs);

        // Select the dendrite bouton and add its position to vtkPolyData
        const std::shared_ptr<DendriteBouton>& bouton = dendrite->getDendriteBouton();
        PositionPtr boutonPosition = bouton->getPosition();
        pointIDs->InsertNextId(definePoints->InsertNextPoint(boutonPosition->x, boutonPosition->y,
                                                               boutonPosition->z));

        // Recursively process child dendrite branches
        const std::vector<std::shared_ptr<DendriteBranch>>& childBranches = dendrite->getDendriteBranches();
        for (const auto& childBranch : childBranches) {
            addDendriteBranchPositionsToPolyData(renderer, childBranch, definePoints, pointIDs);
        }
    }
}

void addAxonBranchPositionsToPolyData(vtkRenderer* renderer, const std::shared_ptr<AxonBranch>& axonBranch, const vtkSmartPointer<vtkPoints>& definePoints, vtkSmartPointer<vtkIdList> pointIDs) {
    // Add the position of the axon branch to vtkPolyData
    PositionPtr branchPosition = axonBranch->getPosition();
    pointIDs->InsertNextId(definePoints->InsertNextPoint(branchPosition->x, branchPosition->y, branchPosition->z));

    // Iterate over the axons and add their positions to vtkPolyData
    const std::vector<std::shared_ptr<Axon>>& axons = axonBranch->getAxons();
    for (const auto& axon : axons) {
        PositionPtr axonPosition = axon->getPosition();
        addAxonBranchToRenderer(renderer, axonBranch, axon, definePoints, pointIDs);

        // Select the axon bouton and add its position to vtkPolyData
        const std::shared_ptr<AxonBouton>& axonBouton = axon->getAxonBouton();
        PositionPtr axonBoutonPosition = axonBouton->getPosition();
        pointIDs->InsertNextId(definePoints->InsertNextPoint(axonBoutonPosition->x, axonBoutonPosition->y,
                                                               axonBoutonPosition->z));

        // Select the synaptic gap and add its position to vtkPolyData
        const std::shared_ptr<SynapticGap>& synapticGap = axonBouton->getSynapticGap();
        addSynapticGapToRenderer(renderer, synapticGap);

        // Recursively process child axon branches
        const std::vector<std::shared_ptr<AxonBranch>>& childBranches = axon->getAxonBranches();
        for (const auto& childBranch : childBranches) {
            addAxonBranchPositionsToPolyData(renderer, childBranch, definePoints, pointIDs);
        }
    }
}

const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string base64_encode(const unsigned char* data, size_t length) {
    std::string base64;
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];

    while (length--) {
        char_array_3[i++] = *(data++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (i = 0; i < 4; i++) {
                base64 += base64_chars[char_array_4[i]];
            }
            i = 0;
        }
    }

    if (i) {
        for (j = i; j < 3; j++) {
            char_array_3[j] = '\0';
        }

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for (j = 0; j < i + 1; j++) {
            base64 += base64_chars[char_array_4[j]];
        }

        while ((i++ < 3)) {
            base64 += '=';
        }
    }

    return base64;
}

bool convertStringToBool(const std::string& value) {
    // Convert to lowercase for case-insensitive comparison
    std::string lowerValue = value;
    std::transform(lowerValue.begin(), lowerValue.end(), lowerValue.begin(), ::tolower);

    if (lowerValue == "true" || lowerValue == "yes" || lowerValue == "1") {
        return true;
    } else if (lowerValue == "false" || lowerValue == "no" || lowerValue == "0") {
        return false;
    } else {
        // Handle error or throw an exception
        std::cerr << "Invalid boolean string representation: " << value << std::endl;
        return false;
    }
}

/**
 * @brief Main function to initialize, run, and finalize the neuron network simulation.
 * @return int Status code (0 for success, non-zero for failures).
 */
int main() {
    // Initialize logger
    Logger logger("errors.log");

    std::string input = "Hello, World!";
    std::string encoded = base64_encode(reinterpret_cast<const unsigned char*>(input.c_str()), input.length());

    std::cout << "Base64 Encoded: " << encoded << std::endl;
    std::string query;

    // Initialize VTK
    vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
    vtkSmartPointer<vtkRenderWindow> render_window = vtkSmartPointer<vtkRenderWindow>::New();
    render_window->AddRenderer(renderer);
    vtkSmartPointer<vtkRenderWindowInteractor> render_window_interactor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
    render_window_interactor->SetRenderWindow(render_window);

    // Create a vtkPoints object to hold the neuron positions
    vtkSmartPointer<vtkPoints> definePoints = vtkSmartPointer<vtkPoints>::New();

    // Create a vtkCellArray object to hold the lines
    vtkSmartPointer<vtkCellArray> lines = vtkSmartPointer<vtkCellArray>::New();


    try {
        // Read the database connection configuration and simulation configuration
        std::vector<std::string> config_filenames = {"db_connection.conf", "simulation.conf"};
        auto config = read_config(config_filenames);
        std::string connection_string = build_connection_string(config);

        // Get the number of neurons from the configuration
        int num_neurons = std::stoi(config["num_neurons"]);
        int num_pixels = std::stoi(config["num_pixels"]);
        int num_phonels = std::stoi(config["num_phonels"]);
        int num_scentels = std::stoi(config["num_scentels"]);
        int num_vocels = std::stoi(config["num_vocels"]);
        int neuron_points_per_layer = std::stoi(config["neuron_points_per_layer"]);
        int pixel_points_per_layer = std::stoi(config["pixel_points_per_layer"]);
        int phonel_points_per_layer = std::stoi(config["phonel_points_per_layer"]);
        int scentel_points_per_layer = std::stoi(config["scentel_points_per_layer"]);
        int vocel_points_per_layer = std::stoi(config["vocel_points_per_layer"]);
        double proximityThreshold = std::stod(config["proximity_threshold"]);
        bool useDatabase = convertStringToBool(config["use_database"]);

        // Connect to PostgreSQL
        pqxx::connection conn(connection_string);

        // Initialize the database (check and create the required table if needed)
        initialize_database(conn);
        // Start a transaction
        pqxx::work txn(conn);
        // Set the transaction isolation level
        txn.exec("SET TRANSACTION ISOLATION LEVEL SERIALIZABLE;");
        txn.exec("SET lock_timeout = '5s';");

        // Create a single instance of NeuronParameters to access the database
        NeuronParameters params;

        // Create a list of neurons
        std::cout << "Creating neurons..." << std::endl;
        std::vector<std::shared_ptr<Neuron>> neurons;
        neurons.reserve(num_neurons);
        double shiftX = 0.0;
        double shiftY = 0.0;
        double shiftZ = 0.0;
        double newPositionX;
        double newPositionY;
        double newPositionZ;
        std::shared_ptr<Neuron> prevNeuron;
        // Currently the application places neurons in a defined pattern but the pattern is not based on cell growth
        for (int i = 0; i < num_neurons; ++i) {
            auto coords = get_coordinates(i, num_neurons, neuron_points_per_layer);
            if ( i > 0 ) {
                prevNeuron = neurons.back();
                shiftX = std::get<0>(coords);
                shiftY = std::get<1>(coords);
                shiftZ = std::get<2>(coords);
            }

            // std::cout << "Creating neuron " << i << " at (" << shiftX << ", " << shiftY << ", " << shiftZ << ")" << std::endl;
            neurons.emplace_back(std::make_shared<Neuron>(std::make_shared<Position>(shiftX, shiftY, shiftZ)));
            neurons.back()->initialise();
            // Sparsely associate neurons
            if (i > 0 && i % 3 > 0) {
                // First move the required gap closer to the other neuron's dendrite bouton - also need to adjust other components too
                PositionPtr prevDendriteBoutonPosition = prevNeuron->getSoma()->getDendriteBranches()[0]->getDendrites()[0]->getDendriteBouton()->getPosition();
                PositionPtr currentSynapticGapPosition = neurons.back()->getSoma()->getAxonHillock()->getAxon()->getAxonBouton()->getSynapticGap()->getPosition();
                newPositionX = prevDendriteBoutonPosition->x + 0.4;
                newPositionY = prevDendriteBoutonPosition->y + 0.4;
                newPositionZ = prevDendriteBoutonPosition->z + 0.4;
                currentSynapticGapPosition->x = newPositionX;
                currentSynapticGapPosition->y = newPositionY;
                currentSynapticGapPosition->z = newPositionZ;
                PositionPtr currentAxonBoutonPosition = neurons.back()->getSoma()->getAxonHillock()->getAxon()->getAxonBouton()->getPosition();
                newPositionX = newPositionX + 0.4;
                newPositionY = newPositionY + 0.4;
                newPositionZ = newPositionZ + 0.4;
                currentAxonBoutonPosition->x = newPositionX;
                currentAxonBoutonPosition->y = newPositionY;
                currentAxonBoutonPosition->z = newPositionZ;
                PositionPtr currentAxonPosition = neurons.back()->getSoma()->getAxonHillock()->getAxon()->getPosition();
                newPositionX = newPositionX + (currentAxonPosition->x - newPositionX) / 2.0;
                newPositionY = newPositionY + (currentAxonPosition->y - newPositionY) / 2.0;
                newPositionZ = newPositionZ + (currentAxonPosition->z - newPositionZ) / 2.0;
                currentAxonPosition->x = newPositionX;
                currentAxonPosition->y = newPositionY;
                currentAxonPosition->z = newPositionZ;
                // Associate the synaptic gap with the other neuron's dendrite bouton if it is now close enough
                associateSynapticGap(*neurons[i - 1], *neurons[i], proximityThreshold);
            }
        }
        std::cout << "Created " << neurons.size() << " neurons." << std::endl;
        // Create a collection of visual inputs (pair of eyes)
        std::cout << "Creating visual sensory inputs..." << std::endl;
        std::vector<std::vector<std::shared_ptr<SensoryReceptor>>> visualInputs(2); // Resize the vector to contain 2 elements        visualInputs[0].reserve(num_pixels / 2);
        visualInputs[1].reserve(num_pixels / 2);
        std::shared_ptr<SensoryReceptor> prevReceptor;
#pragma omp parallel for
        for (int j = 0; j < 2; ++j) {
            for (int i = 0; i < (num_pixels / 2); ++i) {
                auto coords = get_coordinates(i, num_pixels, pixel_points_per_layer);
                if (i > 0) {
                    prevReceptor = visualInputs[j].back();
                    shiftX = std::get<0>(coords) - 100 + (j * 200);
                    shiftY = std::get<1>(coords);
                    shiftZ = std::get<2>(coords) - 100;
                }

                // std::cout << "Creating visual (" << j << ") input " << i << " at (" << shiftX << ", " << shiftY << ", " << shiftZ << ")" << std::endl;
                visualInputs[j].emplace_back(std::make_shared<SensoryReceptor>(std::make_shared<Position>(shiftX, shiftY, shiftZ)));
                visualInputs[j].back()->initialise();
                // Sparsely associate neurons
                if (i > 0 && i % 7 > 0) {
                    // First move the required gap closer to the other neuron's dendrite bouton - also need to adjust other components too
                    PositionPtr currentDendriteBoutonPosition = neurons[int(i + ((num_pixels / 2) * j))]->getSoma()->getDendriteBranches()[0]->getDendrites()[0]->getDendriteBouton()->getPosition();
                    PositionPtr currentSynapticGapPosition = visualInputs[j].back()->getSynapticGaps()[0]->getPosition();
                    newPositionX = currentSynapticGapPosition->x + 0.4;
                    newPositionY = currentSynapticGapPosition->y + 0.4;
                    newPositionZ = currentSynapticGapPosition->z + 0.4;
                    currentDendriteBoutonPosition->x = newPositionX;
                    currentDendriteBoutonPosition->y = newPositionY;
                    currentDendriteBoutonPosition->z = newPositionZ;
                    PositionPtr currentDendritePosition = neurons[int(i + ((num_pixels / 2) * j))]->getSoma()->getDendriteBranches()[0]->getDendrites()[0]->getPosition();
                    newPositionX = newPositionX + 0.4;
                    newPositionY = newPositionY + 0.4;
                    newPositionZ = newPositionZ + 0.4;
                    currentDendritePosition->x = newPositionX;
                    currentDendritePosition->y = newPositionY;
                    currentDendritePosition->z = newPositionZ;
                    // Associate the synaptic gap with the other neuron's dendrite bouton if it is now close enough
                    associateSynapticGap(*visualInputs[j].back(), *neurons[int(i + ((num_pixels / 2) * j))], proximityThreshold);
                }
            }
        }
        std::cout << "Created " << ( visualInputs[0].size() + visualInputs[1].size()) << " sensory receptors." << std::endl;

        // Create a collection of audio inputs (pair of ears)
        std::cout << "Creating auditory sensory inputs..." << std::endl;
        std::vector<std::vector<std::shared_ptr<SensoryReceptor>>> audioInputs(2); // Resize the vector to contain 2 elements
        audioInputs[0].reserve(num_phonels);
        audioInputs[1].reserve(num_phonels);
#pragma omp parallel for
        for (int j = 0; j < 2; ++j) {
            for (int i = 0; i < (num_phonels / 2); ++i) {
                auto coords = get_coordinates(i, num_phonels, phonel_points_per_layer);
                if (i > 0) {
                    prevReceptor = audioInputs[j].back();
                    shiftX = std::get<0>(coords) - 150 + (j * 300);
                    shiftY = std::get<1>(coords);
                    shiftZ = std::get<2>(coords);
                }

                // std::cout << "Creating auditory (" << j << ") input " << i << " at (" << shiftX << ", " << shiftY << ", " << shiftZ << ")" << std::endl;
                audioInputs[j].emplace_back(std::make_shared<SensoryReceptor>(std::make_shared<Position>(shiftX, shiftY, shiftZ)));
                audioInputs[j].back()->initialise();
                // Sparsely associate neurons
                if (i > 0 && i % 11 > 0) {
                    // First move the required gap closer to the other neuron's dendrite bouton - also need to adjust other components too
                    PositionPtr currentDendriteBoutonPosition = neurons[int(i + ((num_phonels / 2) * j))]->getSoma()->getDendriteBranches()[0]->getDendrites()[0]->getDendriteBouton()->getPosition();
                    PositionPtr currentSynapticGapPosition = audioInputs[j].back()->getSynapticGaps()[0]->getPosition();
                    newPositionX = currentSynapticGapPosition->x + 0.4;
                    newPositionY = currentSynapticGapPosition->y + 0.4;
                    newPositionZ = currentSynapticGapPosition->z + 0.4;
                    currentDendriteBoutonPosition->x = newPositionX;
                    currentDendriteBoutonPosition->y = newPositionY;
                    currentDendriteBoutonPosition->z = newPositionZ;
                    PositionPtr currentDendritePosition = neurons[int(i + ((num_phonels / 2) * j))]->getSoma()->getDendriteBranches()[0]->getDendrites()[0]->getPosition();
                    newPositionX = newPositionX + 0.4;
                    newPositionY = newPositionY + 0.4;
                    newPositionZ = newPositionZ + 0.4;
                    currentDendritePosition->x = newPositionX;
                    currentDendritePosition->y = newPositionY;
                    currentDendritePosition->z = newPositionZ;
                    // Associate the synaptic gap with the other neuron's dendrite bouton if it is now close enough
                    associateSynapticGap(*audioInputs[j].back(), *neurons[int(i + ((num_phonels / 2) * j))], proximityThreshold);
                }
            }
        }
        std::cout << "Created " << ( audioInputs[0].size() + audioInputs[1].size()) << " sensory receptors." << std::endl;

        // Create a collection of olfactory inputs (pair of nostrils)
        std::cout << "Creating olfactory sensory inputs..." << std::endl;
        std::vector<std::vector<std::shared_ptr<SensoryReceptor>>> olfactoryInputs(2); // Resize the vector to contain 2 elements
        olfactoryInputs[0].reserve(num_scentels);
        olfactoryInputs[1].reserve(num_scentels);
#pragma omp parallel for
        for (int j = 0; j < 2; ++j) {
            for (int i = 0; i < (num_scentels / 2); ++i) {
                auto coords = get_coordinates(i, num_scentels, scentel_points_per_layer);
                if (i > 0) {
                    prevReceptor = olfactoryInputs[j].back();
                    shiftX = std::get<0>(coords) - 20 + (j * 40);
                    shiftY = std::get<1>(coords) - 10;
                    shiftZ = std::get<2>(coords) - 10;
                }

                // std::cout << "Creating olfactory (" << j << ") input " << i << " at (" << shiftX << ", " << shiftY << ", " << shiftZ << ")" << std::endl;
                olfactoryInputs[j].emplace_back(std::make_shared<SensoryReceptor>(std::make_shared<Position>(shiftX, shiftY, shiftZ)));
                olfactoryInputs[j].back()->initialise();
                // Sparsely associate neurons
                if (i > 0 && i % 13 > 0) {
                    // First move the required gap closer to the other neuron's dendrite bouton - also need to adjust other components too
                    PositionPtr currentDendriteBoutonPosition = neurons[int(i + ((num_scentels / 2) * j))]->getSoma()->getDendriteBranches()[0]->getDendrites()[0]->getDendriteBouton()->getPosition();
                    PositionPtr currentSynapticGapPosition = olfactoryInputs[j].back()->getSynapticGaps()[0]->getPosition();
                    newPositionX = currentSynapticGapPosition->x + 0.4;
                    newPositionY = currentSynapticGapPosition->y + 0.4;
                    newPositionZ = currentSynapticGapPosition->z + 0.4;
                    currentDendriteBoutonPosition->x = newPositionX;
                    currentDendriteBoutonPosition->y = newPositionY;
                    currentDendriteBoutonPosition->z = newPositionZ;
                    PositionPtr currentDendritePosition = neurons[int(i + ((num_scentels / 2) * j))]->getSoma()->getDendriteBranches()[0]->getDendrites()[0]->getPosition();
                    newPositionX = newPositionX + 0.4;
                    newPositionY = newPositionY + 0.4;
                    newPositionZ = newPositionZ + 0.4;
                    currentDendritePosition->x = newPositionX;
                    currentDendritePosition->y = newPositionY;
                    currentDendritePosition->z = newPositionZ;
                    // Associate the synaptic gap with the other neuron's dendrite bouton if it is now close enough
                    associateSynapticGap(*olfactoryInputs[j].back(), *neurons[int(i + ((num_scentels / 2) * j))], proximityThreshold);
                }
            }
        }
        std::cout << "Created " << ( olfactoryInputs[0].size() + olfactoryInputs[1].size()) << " sensory receptors." << std::endl;

        // Create a collection of vocal effector outputs (vocal tract cords/muscle control)
        std::cout << "Creating vocal effector outputs..." << std::endl;
        std::vector<std::shared_ptr<Effector>> vocalOutputs;
        vocalOutputs.reserve(num_vocels);
        std::shared_ptr<Effector> prevEffector;

        for (int i = 0; i < (num_vocels); ++i) {
            auto coords = get_coordinates(i, num_vocels, vocel_points_per_layer);
            if (i > 0) {
                prevEffector = vocalOutputs.back();
                shiftX = std::get<0>(coords);
                shiftY = std::get<1>(coords) - 100;
                shiftZ = std::get<2>(coords) + 10;
            }

            // std::cout << "Creating vocal output " << i << " at (" << shiftX << ", " << shiftY << ", " << shiftZ << ")" << std::endl;
            vocalOutputs.emplace_back(std::make_shared<Effector>(std::make_shared<Position>(shiftX, shiftY, shiftZ)));
            vocalOutputs.back()->initialise();
            // Sparsely associate neurons
            if (i > 0 && i % 17 > 0 && !neurons[int(i + num_vocels)]->getSoma()->getAxonHillock()->getAxon()->getAxonBouton()->getSynapticGap()->isAssociated() ) {
                // First move the required gap closer to the other neuron's dendrite bouton - also need to adjust other components too
                PositionPtr currentSynapticGapPosition = neurons[int(i + num_vocels)]->getSoma()->getAxonHillock()->getAxon()->getAxonBouton()->getSynapticGap()->getPosition();
                PositionPtr currentEffectorPosition = vocalOutputs.back()->getPosition();
                newPositionX = currentEffectorPosition->x - 0.4;
                newPositionY = currentEffectorPosition->y - 0.4;
                newPositionZ = currentEffectorPosition->z - 0.4;
                currentSynapticGapPosition->x = newPositionX;
                currentSynapticGapPosition->y = newPositionY;
                currentSynapticGapPosition->z = newPositionZ;
                PositionPtr currentAxonBoutonPosition = neurons[int(i + num_vocels)]->getSoma()->getAxonHillock()->getAxon()->getAxonBouton()->getPosition();
                newPositionX = newPositionX + 0.4;
                newPositionY = newPositionY + 0.4;
                newPositionZ = newPositionZ + 0.4;
                currentAxonBoutonPosition->x = newPositionX;
                currentAxonBoutonPosition->y = newPositionY;
                currentAxonBoutonPosition->z = newPositionZ;
                PositionPtr currentAxonPosition = neurons[int(i + num_vocels)]->getSoma()->getAxonHillock()->getAxon()->getPosition();
                newPositionX = newPositionX + 0.4;
                newPositionY = newPositionY + 0.4;
                newPositionZ = newPositionZ + 0.4;
                currentAxonPosition->x = newPositionX;
                currentAxonPosition->y = newPositionY;
                currentAxonPosition->z = newPositionZ;
                // Associate the synaptic gap with the other neuron's dendrite bouton if it is now close enough
                neurons[int(i + num_vocels)]->getSoma()->getAxonHillock()->getAxon()->getAxonBouton()->getSynapticGap()->setAsAssociated();
            }
        }
        std::cout << "Created " << vocalOutputs.size() << " effectors." << std::endl;

// Assuming neurons is a std::vector<Neuron*> or similar container
// and associateSynapticGap is a function you've defined elsewhere

// Parallelize synaptic association
#pragma omp parallel for schedule(dynamic)
        for (size_t i = 0; i < neurons.size(); ++i) {
            for (size_t j = i + 1; j < neurons.size(); ++j) {
                associateSynapticGap(*neurons[i], *neurons[j], proximityThreshold);
            }
        }

// Use a fixed number of threads
        const size_t numThreads = std::thread::hardware_concurrency();

        std::vector<std::thread> threads;
        threads.reserve(numThreads);

        size_t neuronsPerThread = neurons.size() / numThreads;

// Calculate the propagation rate in parallel
        for (size_t t = 0; t < numThreads; ++t) {
            size_t start = t * neuronsPerThread;
            size_t end = (t + 1) * neuronsPerThread;
            if (t == numThreads - 1) end = neurons.size(); // Handle remainder

            threads.emplace_back([=, &neurons]() {
                for (size_t i = start; i < end; ++i) {
                    computePropagationRate(neurons[i]);
                }
            });
        }

// Join the threads
        for (std::thread& t : threads) {
            t.join();
        }

        // Get the total propagation rate
        double propagationRate = totalPropagationRate.load();

        std::cout << "The propagation rate is " << propagationRate << std::endl;

        int neuron_id = 0;
        int soma_id = 0;
        int axon_hillock_id = 0;
        int axon_id = 0;
        int axon_bouton_id = 0;
        int axon_branch_id = 0;
        int synaptic_gap_id = 0;
        int dendrite_id = 0;
        int dendrite_bouton_id = 0;
        int dendrite_branch_id = 0;

        for (auto& neuron : neurons) {
            query = "INSERT INTO neurons (neuron_id, x, y, z) VALUES (" + std::to_string(neuron_id) + ", " + std::to_string(neuron->getPosition()->x) + ", " + std::to_string(neuron->getPosition()->y) + ", " + std::to_string(neuron->getPosition()->z) + ")";
            std::cout << query << std::endl;
            txn.exec(query);
            query = "INSERT INTO somas (soma_id, neuron_id, x, y, z) VALUES (" + std::to_string(soma_id) + ", " + std::to_string(neuron_id) + ", " + std::to_string(neuron->getSoma()->getPosition()->x) + ", " + std::to_string(neuron->getSoma()->getPosition()->y) + ", " + std::to_string(neuron->getSoma()->getPosition()->z) + ")";
            std::cout << query << std::endl;
            txn.exec(query);
            query = "INSERT INTO axonhillocks (axon_hillock_id, soma_id, x, y, z) VALUES (" + std::to_string(axon_hillock_id) + ", " + std::to_string(soma_id) + ", " + std::to_string(neuron->getSoma()->getAxonHillock()->getPosition()->x) + ", " + std::to_string(neuron->getSoma()->getAxonHillock()->getPosition()->y) + ", " + std::to_string(neuron->getSoma()->getAxonHillock()->getPosition()->z) + ")";
            std::cout << query << std::endl;
            txn.exec(query);
            query = "INSERT INTO axons (axon_id, axon_hillock_id, x, y, z) VALUES (" + std::to_string(axon_id) + ", " + std::to_string(axon_hillock_id) + ", " + std::to_string(neuron->getSoma()->getAxonHillock()->getAxon()->getPosition()->x) + ", " + std::to_string(neuron->getSoma()->getAxonHillock()->getAxon()->getPosition()->y) + ", " + std::to_string(neuron->getSoma()->getAxonHillock()->getAxon()->getPosition()->z) + ")";
            std::cout << query << std::endl;
            txn.exec(query);
            query = "INSERT INTO axonboutons (axon_bouton_id, axon_id, x, y, z) VALUES (" + std::to_string(axon_bouton_id) + ", " + std::to_string(axon_id) + ", " + std::to_string(neuron->getSoma()->getAxonHillock()->getAxon()->getAxonBouton()->getPosition()->x) + ", " + std::to_string(neuron->getSoma()->getAxonHillock()->getAxon()->getAxonBouton()->getPosition()->y) + ", " + std::to_string(neuron->getSoma()->getAxonHillock()->getAxon()->getAxonBouton()->getPosition()->z) + ")";
            std::cout << query << std::endl;
            txn.exec(query);
            query = "INSERT INTO synapticgaps (synaptic_gap_id, axon_bouton_id, x, y, z) VALUES (" + std::to_string(synaptic_gap_id) + ", " + std::to_string(axon_bouton_id) + ", " + std::to_string(neuron->getSoma()->getAxonHillock()->getAxon()->getAxonBouton()->getSynapticGap()->getPosition()->x) + ", " + std::to_string(neuron->getSoma()->getAxonHillock()->getAxon()->getAxonBouton()->getSynapticGap()->getPosition()->y) + ", " + std::to_string(neuron->getSoma()->getAxonHillock()->getAxon()->getAxonBouton()->getSynapticGap()->getPosition()->z) + ")";
            std::cout << query << std::endl;
            txn.exec(query);
            axon_id++;
            axon_bouton_id++;
            synaptic_gap_id++;
            query = "INSERT INTO dendritebranches (dendrite_branch_id, soma_id, x, y, z) VALUES (" + std::to_string(dendrite_branch_id) + ", " + std::to_string(soma_id) + ", " + std::to_string(neuron->getSoma()->getDendriteBranches()[0]->getPosition()->x) + ", " + std::to_string(neuron->getSoma()->getDendriteBranches()[0]->getPosition()->y) + ", " + std::to_string(neuron->getSoma()->getDendriteBranches()[0]->getPosition()->z) + ")";
            std::cout << query << std::endl;
            txn.exec(query);
            query = "INSERT INTO dendrites (dendrite_id, dendrite_branch_id, x, y, z) VALUES (" + std::to_string(dendrite_id) + ", " + std::to_string(dendrite_branch_id) + ", " + std::to_string(neuron->getSoma()->getDendriteBranches()[0]->getDendrites()[0]->getPosition()->x) + ", " + std::to_string(neuron->getSoma()->getDendriteBranches()[0]->getDendrites()[0]->getPosition()->y) + ", " + std::to_string(neuron->getSoma()->getDendriteBranches()[0]->getDendrites()[0]->getPosition()->z) + ")";
            std::cout << query << std::endl;
            txn.exec(query);
            query = "INSERT INTO dendriteboutons (dendrite_bouton_id, dendrite_id, x, y, z) VALUES (" + std::to_string(dendrite_bouton_id) + ", " + std::to_string(dendrite_id) + ", " + std::to_string(neuron->getSoma()->getDendriteBranches()[0]->getDendrites()[0]->getDendriteBouton()->getPosition()->x) + ", " + std::to_string(neuron->getSoma()->getDendriteBranches()[0]->getDendrites()[0]->getDendriteBouton()->getPosition()->y) + ", " + std::to_string(neuron->getSoma()->getDendriteBranches()[0]->getDendrites()[0]->getDendriteBouton()->getPosition()->z) + ")";
            std::cout << query << std::endl;
            txn.exec(query);
            axon_hillock_id++;
            soma_id++;
            neuron_id++;
            dendrite_branch_id++;
            dendrite_id++;
            dendrite_bouton_id++;
        }

        // Add any additional functionality for virtual volume constraint and relaxation here
        // Save the propagation rate to the database if it's valid (not null)
        if (propagationRate != 0) {
            //std::string query = "INSERT INTO neurons (propagation_rate, neuron_type, axon_length) VALUES (" + std::to_string(propagationRate) + ", 0, 0)";
            //txn.exec(query);
            txn.commit();
        } else {
            throw std::runtime_error("The propagation rate is not valid. Skipping database insertion.");
        }



    } catch (const std::exception &e) {
        logger << "Error: " << e.what() << std::endl;
        std::cerr << "Error: " << e.what() << std::endl;
    } catch (...) {
        logger << "Error: Unknown exception occurred." << std::endl;
        std::cerr << "Error: Unknown exception occurred." << std::endl;
    }

    return 0;
}
