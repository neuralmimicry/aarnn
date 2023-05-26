/**
 * @file  aarnn.cpp
 * @brief This file contains the implementation of a neuronal network simulation.
 * @author Paul B. Isaac's
 * @version 0.1
 * @date  12-May-2023
 */
#include <iostream>
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

/*!
 * The VTK_MODULE_INIT is definitely required. Without it NULL is returned to ::New() type calls
 */
#include <vtkAutoInit.h>
VTK_MODULE_INIT(vtkRenderingOpenGL2); //! VTK was built with vtkRenderingOpenGL2
VTK_MODULE_INIT(vtkRenderingFreeType); //!
VTK_MODULE_INIT(vtkInteractionStyle); //!
#include <vtkVersion.h>
#include <vtkActor.h>
#include <vtkActor2D.h>
#include <vtkCamera.h>
#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkColorTransferFunction.h>
#include <vtkContourFilter.h>
#include <vtkCoordinate.h>
#include <vtkFloatArray.h>
#include <vtkMath.h>
#include <vtkParametricFunctionSource.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolygon.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkPolyDataMapper2D.h>
#include <vtkAppendPolyData.h>
#include <vtkProgrammableSource.h>
#include <vtkProperty.h>
#include <vtkProperty2D.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkReverseSense.h>
#include <vtkSmartPointer.h>
#include <vtkSurfaceReconstructionFilter.h>
#include <vtkTextActor.h>
#include <vtkTextMapper.h>
#include <vtkTextProperty.h>
#include <vtkUnsignedCharArray.h>
#include <vtkVertexGlyphFilter.h>
#include <vtkXMLPolyDataWriter.h>
#include <cmath>

#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>

#include "aarnn.h"
#include "dendrite.h"
#include "dendritebranch.h"

/*
Setup VTK environment
 */

vtkSmartPointer<vtkRenderWindow> render_window;
vtkSmartPointer<vtkRenderWindowInteractor> render_window_interactor;

vtkSmartPointer<vtkPoints> define_points = vtkSmartPointer<vtkPoints>::New();
std::vector<vtkSmartPointer<vtkCellArray>> define_cellarrays;
std::vector<vtkSmartPointer<vtkPolyData>> define_polydata;

std::vector<vtkSmartPointer<vtkSurfaceReconstructionFilter>> define_surfaces;
std::vector<vtkSmartPointer<vtkContourFilter>> define_contourfilters;
std::vector<vtkSmartPointer<vtkReverseSense>> define_reversals;
std::vector<vtkSmartPointer<vtkPolyDataMapper>> define_datamappers;
std::vector<vtkSmartPointer<vtkPolyDataMapper2D>> define_datamappers2D;
std::vector<vtkSmartPointer<vtkActor>> define_actors;
std::vector<vtkSmartPointer<vtkActor2D>> define_actors2D;
std::vector<vtkSmartPointer<vtkTextActor>> define_textactors;

std::vector<vtkSmartPointer<vtkRenderer>> define_renderers;

static vtkSmartPointer<vtkPolyData> TransformBack(vtkSmartPointer<vtkPoints> pt, vtkSmartPointer<vtkPolyData> pd);

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
    double r = (double)(layer + 1) / (total_points / points_per_layer);

    // azimuthal angle based on golden angle
    double theta = golden_angle * index_in_layer;

    // height y is a function of layer
    double y = 1 - (double)(layer) / (total_points / points_per_layer - 1);

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
    double distanceTo(const Position& other) const {
        double diff[3] = { x - other.x, y - other.y, z - other.z };
        return vtkMath::Norm(diff);
    }

    // Function to set the position coordinates
    void set(double newX, double newY, double newZ) {
        x = newX;
        y = newY;
        z = newZ;
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
class NeuronComponent {
public:
    using PositionPtr = std::shared_ptr<PositionType>;

    explicit NeuronComponent(const PositionPtr& position) : position(position) {}
    virtual ~NeuronComponent() = default;

    [[nodiscard]] const PositionPtr& getPosition() const {
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
class NeuronShapeComponent {
public:
    using ShapePtr = std::shared_ptr<Shape3D>;

    NeuronShapeComponent() = default;
    virtual ~NeuronShapeComponent() = default;

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
class SynapticGap : public std::enable_shared_from_this<SynapticGap>, public NeuronComponent<Position>, public NeuronShapeComponent {
public:
    explicit SynapticGap(const PositionPtr& position) : NeuronComponent(position), NeuronShapeComponent() {
        // On construction set a default propagation rate
        propagationRate = 0.5;
    }
    ~SynapticGap() override = default;
    // Method to check if the SynapticGap has been associated
    bool isAssociated() const {
        return associated;
    }
    // Method to set the SynapticGap as associated
    void setAsAssociated() {
        associated = true;
    }

    void updateFromSynapticBouton(std::shared_ptr<SynapticBouton> parentSynapticBoutonPointer) { parentSynapticBouton = std::move(parentSynapticBoutonPointer); }

    [[nodiscard]] std::shared_ptr<SynapticBouton> getParentSynapticBouton() const { return parentSynapticBouton; }

    void updateFromDendriteBouton(std::shared_ptr<DendriteBouton> parentDendriteBoutonPointer) { parentDendriteBouton = std::move(parentDendriteBoutonPointer); }

    [[nodiscard]] std::shared_ptr<DendriteBouton> getParentDendriteBouton() const { return parentDendriteBouton; }

private:
    bool associated = false;  // Initially, the SynapticGap is not associated with a DendriteBouton
    std::shared_ptr<SynapticBouton> parentSynapticBouton;
    std::shared_ptr<DendriteBouton> parentDendriteBouton;
};


// Dendrite Bouton class
class DendriteBouton : public std::enable_shared_from_this<DendriteBouton>, public NeuronComponent<Position>, public NeuronShapeComponent {
public:
    explicit DendriteBouton(const PositionPtr& position) : NeuronComponent(position), NeuronShapeComponent() {
        // On construction set a default propagation rate
        propagationRate = 0.5;
    }
    ~DendriteBouton() override = default;

    void connectSynapticGap(const std::shared_ptr<SynapticGap>& gap) {
        onwardSynapticGap = gap;
    }

    [[nodiscard]] std::shared_ptr<SynapticGap> getOnwardSynapticGap() const {
        return onwardSynapticGap;
    }

    double getPropagationRate() override {
        // Implement the calculation based on the dendrite bouton properties
        return propagationRate;
    }

    Position updatePosition(const PositionPtr& newPosition) {
        position = newPosition;
        return *position;
    }

    void updateFromDendrite(std::shared_ptr<Dendrite> parentDendritePointer) { parentDendrite = std::move(parentDendritePointer); }

    [[nodiscard]] std::shared_ptr<Dendrite> getParentDendrite() const { return parentDendrite; }

private:
    std::shared_ptr<SynapticGap> onwardSynapticGap{};
    std::shared_ptr<Dendrite> parentDendrite{};
};

// Dendrite class
class Dendrite : public std::enable_shared_from_this<Dendrite>, public NeuronComponent<Position>, public NeuronShapeComponent {
public:
    explicit Dendrite(const PositionPtr& position) : NeuronComponent(position), NeuronShapeComponent() {
        // On construction set a default propagation rate
        propagationRate = 0.5;
        addBouton(std::make_shared<DendriteBouton>(std::make_shared<Position>(position->x, position->y, position->z)));
    }
    ~Dendrite() override = default;

    void addBranch(std::shared_ptr<DendriteBranch> branch);

    [[nodiscard]] const std::vector<std::shared_ptr<DendriteBranch>>& getBranches() const {
        return branches;
    }

    void addBouton(std::shared_ptr<DendriteBouton> bouton) {
        auto coords = get_coordinates(int (boutons.size() + 1), int(boutons.size() + 1), int(5));
        PositionPtr currentPosition = bouton->getPosition();
        double x = std::get<0>(coords) + currentPosition->x;
        double y = std::get<1>(coords) + currentPosition->y;
        double z = std::get<2>(coords) + currentPosition->z;
        currentPosition->x = x;
        currentPosition->y = y;
        currentPosition->z = z;
        boutons.emplace_back(bouton);
    }

    [[nodiscard]] const std::vector<std::shared_ptr<DendriteBouton>>& getBoutons() const {
        return boutons;
    }

    void updateFromDendriteBranch(std::shared_ptr<DendriteBranch> parentDendriteBranchPointer) { parentDendriteBranch = std::move(parentDendriteBranchPointer); }

    [[nodiscard]] std::shared_ptr<DendriteBranch> getParentDendriteBranch() const { return parentDendriteBranch; }

private:
    std::vector<std::shared_ptr<DendriteBranch>> branches;
    std::vector<std::shared_ptr<DendriteBouton>> boutons;
    std::shared_ptr<DendriteBranch> branch{};
    std::shared_ptr<DendriteBouton> bouton{};
    std::shared_ptr<DendriteBranch> parentDendriteBranch{};
};

// Dendrite branch class
class DendriteBranch : public std::enable_shared_from_this<DendriteBranch>, public NeuronComponent<Position>, public NeuronShapeComponent {
public:
    explicit DendriteBranch(const PositionPtr& position) : NeuronComponent(position), NeuronShapeComponent() {
        // On construction set a default propagation rate
        propagationRate = 0.5;
        connectDendrite(std::make_shared<Dendrite>(std::make_shared<Position>(position->x, position->y, position->z)));
    }
    ~DendriteBranch() override = default;

    void connectDendrite(std::shared_ptr<Dendrite> dendrite) {
        auto coords = get_coordinates(int (onwardDendrites.size() + 1), int(onwardDendrites.size() + 1), int(5));
        PositionPtr currentPosition = dendrite->getPosition();
        double x = std::get<0>(coords) + currentPosition->x;
        double y = std::get<1>(coords) + currentPosition->y;
        double z = std::get<2>(coords) + currentPosition->z;
        currentPosition->x = x;
        currentPosition->y = y;
        currentPosition->z = z;
        onwardDendrites.emplace_back(dendrite);
    }

    [[nodiscard]] std::vector<std::shared_ptr<Dendrite>> getOnwardDendrites() const {
        return onwardDendrites;
    }

    void updateFromSoma(std::shared_ptr<Soma> parentSomaPointer) { parentSoma = std::move(parentSomaPointer); }

    [[nodiscard]] std::shared_ptr<Soma> getParentSoma() const { return parentSoma; }

    void updateFromDendrite(std::shared_ptr<Dendrite> parentDendritePointer) { parentDendrite = std::move(parentDendritePointer); }

    [[nodiscard]] std::shared_ptr<Dendrite> getParentDendrite() const { return parentDendrite; }

private:
    std::vector<std::shared_ptr<Dendrite>> onwardDendrites;
    std::shared_ptr<Soma> parentSoma;
    std::shared_ptr<Dendrite> parentDendrite;
};

void Dendrite::addBranch(std::shared_ptr<DendriteBranch> branch) {
    auto coords = get_coordinates(int (branches.size() + 1), int(branches.size() + 1), int(5));
    PositionPtr currentPosition = branch->getPosition();
    double x = std::get<0>(coords) + currentPosition->x;
    double y = std::get<1>(coords) + currentPosition->y;
    double z = std::get<2>(coords) + currentPosition->z;
    currentPosition->x = x;
    currentPosition->y = y;
    currentPosition->z = z;
    branches.emplace_back(branch);
}

// Synaptic bouton class
class SynapticBouton : public std::enable_shared_from_this<SynapticBouton>, public NeuronComponent<Position>, public NeuronShapeComponent {
public:
    explicit SynapticBouton(const PositionPtr& position);
    ~SynapticBouton() override = default;

    void connectSynapticGap(std::shared_ptr<SynapticGap> gap);

    [[nodiscard]] std::shared_ptr<SynapticGap> getOnwardSynapticGap() const {
        return onwardSynapticGap;
    }

    void setNeuron(std::weak_ptr<Neuron> neuron) {
        this->neuron = neuron;
    }

    void updateFromAxon(std::shared_ptr<Axon> parentAxonPointer) { parentAxon = std::move(parentAxonPointer); }

    [[nodiscard]] std::shared_ptr<Axon> getParentAxon() const { return parentAxon; }

private:
    std::shared_ptr<SynapticGap> onwardSynapticGap{};
    std::weak_ptr<Neuron> neuron;
    std::shared_ptr<Axon> parentAxon;
};

// Axon class
class Axon : public std::enable_shared_from_this<Axon>, public NeuronComponent<Position>, public NeuronShapeComponent {
public:
    explicit Axon(const PositionPtr& position)
            : NeuronComponent(position),
              NeuronShapeComponent() {
        // On construction set a default propagation rate
        propagationRate = 0.5;
        onwardSynapticBouton = std::make_shared<SynapticBouton>(std::make_shared<Position>(position->x, position->y, position->z));
    }
    ~Axon() override = default;

    void addBranch(std::shared_ptr<AxonBranch> branch);

    [[nodiscard]] const std::vector<std::shared_ptr<AxonBranch>>& getBranches() const {
        return branches;
    }

    [[nodiscard]] std::shared_ptr<SynapticBouton> getOnwardSynapticBouton() const {
        return onwardSynapticBouton;
    }

    void updateFromAxonHillock(std::shared_ptr<AxonHillock> parentAxonHillockPointer) { parentAxonHillock = std::move(parentAxonHillockPointer); }

    [[nodiscard]] std::shared_ptr<AxonHillock> getParentAxonHillock() const { return parentAxonHillock; }

    void updateFromAxonBranch(std::shared_ptr<AxonBranch> parentAxonBranchPointer) { parentAxonBranch = std::move(parentAxonBranchPointer); }

    [[nodiscard]] std::shared_ptr<AxonBranch> getParentAxonBranch() const { return parentAxonBranch; }

private:
    std::vector<std::shared_ptr<AxonBranch>> branches;
    std::shared_ptr<SynapticBouton> onwardSynapticBouton;
    std::shared_ptr<AxonHillock> parentAxonHillock;
    std::shared_ptr<AxonBranch> parentAxonBranch;
};

// Axon branch class
class AxonBranch : public std::enable_shared_from_this<AxonBranch>, public NeuronComponent<Position>, public NeuronShapeComponent {
public:
    explicit AxonBranch(const PositionPtr& position) : NeuronComponent(position), NeuronShapeComponent() {
        // On construction set a default propagation rate
        propagationRate = 0.5;
        addAxon(std::make_shared<Axon>(std::make_shared<Position>(position->x, position->y, position->z)));
    }
    ~AxonBranch() override = default;

    void addAxon(std::shared_ptr<Axon> axon) {
        auto coords = get_coordinates(int (axons.size() + 1), int(axons.size() + 1), int(5));
        PositionPtr currentPosition = axon->getPosition();
        double x = std::get<0>(coords) + currentPosition->x;
        double y = std::get<1>(coords) + currentPosition->y;
        double z = std::get<2>(coords) + currentPosition->z;
        currentPosition->x = x;
        currentPosition->y = y;
        currentPosition->z = z;
        axons.emplace_back(axon);
    }

    [[nodiscard]] const std::vector<std::shared_ptr<Axon>>& getAxons() const {
        return axons;
    }
    void updateFromAxon(std::shared_ptr<Axon> parentPointer) { parentAxon = std::move(parentPointer); }

    [[nodiscard]] std::shared_ptr<Axon> getParentAxon() const { return parentAxon; }

private:
    std::vector<std::shared_ptr<Axon>> axons;
    std::shared_ptr<Axon> parentAxon;
};

void Axon::addBranch(std::shared_ptr<AxonBranch> branch) {
    auto coords = get_coordinates(int (branches.size() + 1), int(branches.size() + 1), int(5));
    PositionPtr currentPosition = branch->getPosition();
    double x = std::get<0>(coords) + currentPosition->x;
    double y = std::get<1>(coords) + currentPosition->y;
    double z = std::get<2>(coords) + currentPosition->z;
    currentPosition->x = x;
    currentPosition->y = y;
    currentPosition->z = z;
    branches.emplace_back(branch);
}


// Axon hillock class
class AxonHillock : public std::enable_shared_from_this<AxonHillock>, public NeuronComponent<Position>, public NeuronShapeComponent {
public:
    explicit AxonHillock(const PositionPtr& position) : NeuronComponent(position), NeuronShapeComponent() {
        // On construction set a default propagation rate
        propagationRate = 0.5;
        addAxon(std::make_shared<Axon>(std::make_shared<Position>(position->x, position->y, position->z)));
    }
    ~AxonHillock() override = default;

    void addAxon(std::shared_ptr<Axon> axon) {
        auto coords = get_coordinates(int (1), int(1), int(5));
        PositionPtr currentPosition = axon->getPosition();
        double x = std::get<0>(coords) + currentPosition->x;
        double y = std::get<1>(coords) + currentPosition->y;
        double z = std::get<2>(coords) + currentPosition->z;
        currentPosition->x = x;
        currentPosition->y = y;
        currentPosition->z = z;
        onwardAxon = axon;
    }

    [[nodiscard]] std::shared_ptr<Axon> getOnwardAxon() const {
        return onwardAxon;
    }

    void updateFromSoma(std::shared_ptr<Soma> parentPointer) { parentSoma = std::move(parentPointer); }

    [[nodiscard]] std::shared_ptr<Soma> getParentSoma() const { return parentSoma; }

private:
    std::shared_ptr<Axon> onwardAxon{};
    std::shared_ptr<Soma> parentSoma;
};

class Soma : public std::enable_shared_from_this<Soma>, public NeuronComponent<Position>, public NeuronShapeComponent {
public:
    explicit Soma(const PositionPtr& position) : NeuronComponent(position), NeuronShapeComponent() {
        // On construction set a default propagation rate
        propagationRate = 0.5;
        addAxonHillock(std::make_shared<AxonHillock>(std::make_shared<Position>(position->x, position->y, position->z)));
        addDendriteBranch(std::make_shared<DendriteBranch>(std::make_shared<Position>(position->x, position->y, position->z)));
    }
    ~Soma() override = default;

    void addAxonHillock(std::shared_ptr<AxonHillock> axonHillock) {
        auto coords = get_coordinates(int (1), int(1), int(5));
        PositionPtr currentPosition = axonHillock->getPosition();
        double x = std::get<0>(coords) + currentPosition->x;
        double y = std::get<1>(coords) + currentPosition->y;
        double z = std::get<2>(coords) + currentPosition->z;
        currentPosition->x = x;
        currentPosition->y = y;
        currentPosition->z = z;
        onwardAxonHillock = std::move(axonHillock);
    }

    [[nodiscard]] std::shared_ptr<AxonHillock> getOnwardAxonHillock() const {
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

private:
    std::vector<std::shared_ptr<SynapticGap>> synapticGaps;
    std::vector<std::shared_ptr<DendriteBranch>> dendriteBranches;
    std::shared_ptr<AxonHillock> onwardAxonHillock{};
    std::shared_ptr<Neuron> parentNeuron{};
};

// Now that Soma is fully defined, we can define the member functions of SynapticBouton that use it
SynapticBouton::SynapticBouton(const PositionPtr& position)
        : NeuronComponent(position), NeuronShapeComponent()
{
    // On construction set a default propagation rate
    propagationRate = 0.5;
    connectSynapticGap(std::make_shared<SynapticGap>(std::make_shared<Position>(position->x, position->y, position->z)));
}

/**
 * @brief Neuron class representing a neuron in the simulation.
 */
class Neuron : public std::enable_shared_from_this<Neuron>, public NeuronComponent<Position>, public NeuronShapeComponent {
public:
    explicit Neuron(const PositionPtr& position) : NeuronComponent(position), NeuronShapeComponent() {
        /**
         * @brief Construct a new Neuron object.
         * @param soma Shared pointer to the Soma object of the neuron.
         */
        soma = std::make_shared<Soma>(std::make_shared<Position>(position->x, position->y, position->z));

        registerSomaChild(soma);
    }
    /**
     * @brief Destroy the Neuron object.
     */
    ~Neuron() = default;

    void registerSomaChild(std::shared_ptr<Soma> somaChild) {
        somaChildren.emplace_back(somaChild);
    }

    void updateSomaChildren() {
        for (std::shared_ptr<Soma> somaChild : somaChildren) {
            somaChild->updateFromNeuron(shared_from_this());
        }
    }

    /**
     * @brief Getter function for the soma of the neuron.
     * @return Shared pointer to the Soma object of the neuron.
     */
    std::shared_ptr<Soma> getSoma() {
        return soma;
    }

    std::shared_ptr<SynapticGap> findSynapticGap(const PositionPtr& positionPtr) {
        // Follow the route of soma, axon hillock, axon, synaptic bouton, until reaching the synaptic gap

        // Get the onward axon hillock from the soma
        std::shared_ptr<AxonHillock> onwardAxonHillock = soma->getOnwardAxonHillock();
        if (!onwardAxonHillock) {
            return nullptr;
        }

        // Get the onward axon from the axon hillock
        std::shared_ptr<Axon> onwardAxon = onwardAxonHillock->getOnwardAxon();
        if (!onwardAxon) {
            return nullptr;
        }

        // Recursively traverse the onward axons and their synaptic boutons
        std::shared_ptr<SynapticGap> gap = traverseAxons(onwardAxon, positionPtr);
        return gap;
    }

    void storeAllSynapticGaps() {
        // Clear the synapticGaps vector before traversing
        synapticGaps.clear();

        // Get the onward axon hillock from the soma
        std::shared_ptr<AxonHillock> onwardAxonHillock = soma->getOnwardAxonHillock();
        if (!onwardAxonHillock) {
            return;
        }

        // Get the onward axon from the axon hillock
        std::shared_ptr<Axon> onwardAxon = onwardAxonHillock->getOnwardAxon();
        if (!onwardAxon) {
            return;
        }

        // Recursively traverse the onward axons and their synaptic boutons to store all synaptic gaps
        traverseAxonsForStorage(onwardAxon);
    }

    void addSynapticGap(std::shared_ptr<SynapticGap> synapticGap) {
        synapticGaps.emplace_back(std::move(synapticGap));
    }

    [[nodiscard]] const std::vector<std::shared_ptr<SynapticGap>>& getSynapticGaps() const {
        return synapticGaps;
    }

    void addDendriteBouton(std::shared_ptr<DendriteBouton> dendriteBouton) {
        dendriteBoutons.emplace_back(std::move(dendriteBouton));
    }

    [[nodiscard]] const std::vector<std::shared_ptr<DendriteBouton>>& getDendriteBoutons() const {
        return dendriteBoutons;
    }

private:
    std::shared_ptr<Soma> soma;
    std::vector<std::shared_ptr<SynapticGap>> synapticGaps;
    std::vector<std::shared_ptr<DendriteBouton>> dendriteBoutons;
    std::vector<std::shared_ptr<Soma>> somaChildren;

    // Helper function to recursively traverse the axons and find the synaptic gap
    std::shared_ptr<SynapticGap> traverseAxons(std::shared_ptr<Axon> axon, const PositionPtr& positionPtr) {
        // Check if the current axon's bouton has a gap using the same position pointer
        std::shared_ptr<SynapticBouton> bouton = axon->getOnwardSynapticBouton();
        if (bouton) {
            std::shared_ptr<SynapticGap> gap = bouton->getOnwardSynapticGap();
            if (gap->getPosition() == positionPtr) {
                return gap;
            }
        }

        // Recursively traverse the axon's branches and onward axons
        const std::vector<std::shared_ptr<AxonBranch>>& branches = axon->getBranches();
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

    // Helper function to recursively traverse the axons and store all synaptic gaps
    void traverseAxonsForStorage(std::shared_ptr<Axon> axon) {
        // Check if the current axon's bouton has a gap and store it
        std::shared_ptr<SynapticBouton> bouton = axon->getOnwardSynapticBouton();
        if (bouton) {
            std::shared_ptr<SynapticGap> gap = bouton->getOnwardSynapticGap();
            synapticGaps.emplace_back(gap);
        }

        // Recursively traverse the axon's branches and onward axons
        const std::vector<std::shared_ptr<AxonBranch>>& branches = axon->getBranches();
        for (const auto& branch : branches) {
            const std::vector<std::shared_ptr<Axon>>& onwardAxons = branch->getAxons();
            for (const auto& onwardAxon : onwardAxons) {
                traverseAxonsForStorage(onwardAxon);
            }
        }
    }
};

void SynapticBouton::connectSynapticGap(std::shared_ptr<SynapticGap> gap)
{
    onwardSynapticGap = std::move(gap);
    if (auto spt = neuron.lock()) { // has to check if the shared_ptr is still valid
        spt->addSynapticGap(onwardSynapticGap);
    }
}


// Global variables
std::atomic<double> totalPropagationRate(0.0);
std::mutex mtx;

/**
 * @brief Compute the propagation rate of a neuron.
 * @param neuron Pointer to the Neuron object for which the propagation rate is to be calculated.
 */
void computePropagationRate(std::shared_ptr<Neuron> neuron) {
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
    for (auto& gap : neuron1.getSynapticGaps()) {
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


/**
 * @brief Initialize the database, checking and creating the required table if needed.
 * @param conn Reference to the pqxx::connection object.
 */
void initialize_database(pqxx::connection& conn) {
    pqxx::work txn(conn);

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
                "id SERIAL PRIMARY KEY,"
                "propagation_rate REAL NOT NULL,"
                "neuron_type INTEGER NOT NULL,"
                "axon_length REAL NOT NULL"
                ");"
        );

        txn.commit();
        std::cout << "Created table 'neurons'." << std::endl;
    } else {
        std::cout << "Table 'neurons' already exists." << std::endl;
    }
}

void addPositionsToPolyData(std::shared_ptr<DendriteBranch> dendriteBranch, vtkSmartPointer<vtkPoints> define_points, vtkSmartPointer<vtkIdList> point_ids) {
    // Add the position of the dendrite branch to vtkPolyData
    PositionPtr branchPosition = dendriteBranch->getPosition();
    point_ids->InsertNextId(define_points->InsertNextPoint(branchPosition->x, branchPosition->y, branchPosition->z));

    // Iterate over the dendrites and add their positions to vtkPolyData
    const std::vector<std::shared_ptr<Dendrite>> &dendrites = dendriteBranch->getOnwardDendrites();
    for (const auto &dendrite: dendrites) {
        PositionPtr dendritePosition = dendrite->getPosition();
        point_ids->InsertNextId(define_points->InsertNextPoint(dendritePosition->x, dendritePosition->y,
                                                               dendritePosition->z));

        // Iterate over the dendrite boutons and add their positions to vtkPolyData
        const std::vector<std::shared_ptr<DendriteBouton>> &boutons = dendrite->getBoutons();
        for (const auto &bouton: boutons) {
            PositionPtr boutonPosition = bouton->getPosition();
            point_ids->InsertNextId(define_points->InsertNextPoint(boutonPosition->x, boutonPosition->y,
                                                                   boutonPosition->z));
        }

        // Recursively process child dendrite branches
        const std::vector<std::shared_ptr<DendriteBranch>> &childBranches = dendrite->getBranches();
        for (const auto &childBranch: childBranches) {
            addPositionsToPolyData(childBranch, define_points, point_ids);
        }
    }
}

/**
 * @brief Main function to initialize, run, and finalize the neuron network simulation.
 * @return int Status code (0 for success, non-zero for failures).
 */
int main() {
    // Initialize logger
    Logger logger("errors.log");

    // Initialize VTK
    vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
    render_window = vtkSmartPointer<vtkRenderWindow>::New();
    render_window->AddRenderer(renderer);
    render_window_interactor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
    render_window_interactor->SetRenderWindow(render_window);

    // Create a vtkPolyData object to hold the neuron positions
    vtkSmartPointer<vtkPolyData> polydata = vtkSmartPointer<vtkPolyData>::New();
    polydata->SetPoints(define_points);

    // Create a vtkCellArray object to hold the lines
    vtkSmartPointer<vtkCellArray> lines = vtkSmartPointer<vtkCellArray>::New();


    try {
        // Read the database connection configuration and simulation configuration
        std::vector<std::string> config_filenames = {"db_connection.conf", "simulation.conf"};
        auto config = read_config(config_filenames);
        std::string connection_string = build_connection_string(config);

        // Get the number of neurons from the configuration
        int num_neurons = std::stoi(config["num_neurons"]);
        int points_per_layer = std::stoi(config["points_per_layer"]);

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
        double proximityThreshold = std::stod(config["proximity_threshold"]);

        // Create shared instances of neuron components
        PositionPtr sharedNeuronPosition = std::make_shared<Position>(0, 0, 0);

        // Create a list of neurons
        std::vector<std::shared_ptr<Neuron>> neurons;
        neurons.reserve(num_neurons);
        for (int i = 0; i < num_neurons; ++i) {
            auto coords = get_coordinates(i, num_neurons, points_per_layer);
            neurons.emplace_back(std::make_shared<Neuron>(std::make_shared<Position>(std::get<0>(coords), std::get<1>(coords), std::get<2>(coords))));
        }

        // After all neurons have been created...
        for (size_t i = 0; i < neurons.size(); ++i) {
            for (size_t j = i + 1; j < neurons.size(); ++j) {
                associateSynapticGap(*neurons[i], *neurons[j], proximityThreshold);
            }
        }

        // Calculate the propagation rate in parallel
        std::vector<std::thread> threads;
        threads.reserve(neurons.size());  // Pre-allocate the capacity before the loop

        for (auto& neuron : neurons) {
            threads.emplace_back([=](){ computePropagationRate(neuron); });
        }

        // Join the threads
        for (std::thread& t : threads) {
            t.join();
        }

        // Get the total propagation rate
        double propagationRate = totalPropagationRate.load();

            std::cout << "The propagation rate is " << propagationRate << std::endl;

            // Add any additional functionality for virtual volume constraint and relaxation here
            // Save the propagation rate to the database if it's valid (not null)
            if (propagationRate != 0) {
                std::string query = "INSERT INTO neurons (propagation_rate) VALUES (" + std::to_string(propagationRate) + ")";
                txn.exec(query);
                txn.commit();
            } else {
                throw std::runtime_error("The propagation rate is not valid. Skipping database insertion.");
            }

        // Iterate over the neurons and add their positions to the vtkPolyData
        for (const auto& neuron : neurons) {
            const PositionPtr& neuronPosition = neuron->getPosition();

            // Create a vtkIdList to hold the point indices of the line vertices
            vtkSmartPointer<vtkIdList> point_ids = vtkSmartPointer<vtkIdList>::New();
            point_ids->InsertNextId(define_points->InsertNextPoint(neuronPosition->x, neuronPosition->y, neuronPosition->z));

            const PositionPtr& somaPosition = neuron->getSoma()->getPosition();
            point_ids->InsertNextId(define_points->InsertNextPoint(somaPosition->x, somaPosition->y, somaPosition->z));

            const PositionPtr& axonHillockPosition = neuron->getSoma()->getOnwardAxonHillock()->getPosition();
            point_ids->InsertNextId(define_points->InsertNextPoint(axonHillockPosition->x, axonHillockPosition->y, axonHillockPosition->z));

            // Iterate over the dendrite branches and add their positions to the vtkPolyData
            const std::vector<std::shared_ptr<DendriteBranch>>& dendriteBranches = neuron->getSoma()->getDendriteBranches();
            for (const auto& dendriteBranch : dendriteBranches) {
                addPositionsToPolyData(dendriteBranch, define_points, point_ids);
            }

            // Add the line to the cell array
            lines->InsertNextCell(point_ids);
        }

        // Add the cell array to the vtkPolyData
        polydata->SetLines(lines);

        // Create a mapper and actor for the neuron positions
        vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputData(polydata);
        vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper(mapper);
        renderer->AddActor(actor);

        // Set up the render window and start the interaction
        render_window->Render();
        render_window_interactor->Start();

    } catch (const std::exception &e) {
        logger << "Error: " << e.what() << std::endl;
        std::cerr << "Error: " << e.what() << std::endl;
    } catch (...) {
        logger << "Error: Unknown exception occurred." << std::endl;
        std::cerr << "Error: Unknown exception occurred." << std::endl;
    }

    return 0;
}
