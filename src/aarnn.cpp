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
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/bzip2.hpp>
#include <boost/iostreams/filter/zlib.hpp>

extern "C" {
#include <libavcodec/avcodec.h>
}

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
#include <vtkConeSource.h>
#include <vtkContourFilter.h>
#include <vtkCoordinate.h>
#include <vtkCylinderSource.h>
#include <vtkDataSetMapper.h>
#include <vtkDelaunay3D.h>
#include <vtkFloatArray.h>
#include <vtkGeometryFilter.h>
#include <vtkGlyph3D.h>
#include <vtkLineSource.h>
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
#include <vtkSphereSource.h>
#include <vtkSmartPointer.h>
#include <vtkSurfaceReconstructionFilter.h>
#include <vtkTextActor.h>
#include <vtkTextMapper.h>
#include <vtkTextProperty.h>
#include <vtkTubeFilter.h>
#include <vtkUnsignedCharArray.h>
#include <vtkVertexGlyphFilter.h>
#include <vtkWarpTo.h>
#include <vtkXMLPolyDataWriter.h>
#include <cmath>
#include <vtkWindowToImageFilter.h>
#include <vtkJPEGWriter.h>
#include <vtkImageData.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>

#include "aarnn.h"
#include "dendrite.h"
#include "dendritebranch.h"

// websocketpp typedefs
typedef websocketpp::server<websocketpp::config::asio> Server;
typedef websocketpp::connection_hdl ConnectionHandle;

/*
Setup VTK environment
 */

vtkSmartPointer<vtkRenderWindow> renderWindow;
vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor;

vtkSmartPointer<vtkPoints> definePoints = vtkSmartPointer<vtkPoints>::New();
std::vector<vtkSmartPointer<vtkCellArray>> defineCellarrays;
std::vector<vtkSmartPointer<vtkPolyData>> definePolydata;

std::vector<vtkSmartPointer<vtkSurfaceReconstructionFilter>> defineSurfaces;
std::vector<vtkSmartPointer<vtkContourFilter>> defineContourFilters;
std::vector<vtkSmartPointer<vtkReverseSense>> defineReversals;
std::vector<vtkSmartPointer<vtkPolyDataMapper>> defineDataMappers;
std::vector<vtkSmartPointer<vtkPolyDataMapper2D>> defineDataMappers2D;
std::vector<vtkSmartPointer<vtkActor>> defineActors;
std::vector<vtkSmartPointer<vtkActor2D>> defineActors2D;
std::vector<vtkSmartPointer<vtkTextActor>> defineTextActors;

std::vector<vtkSmartPointer<vtkRenderer>> defineRenderers;

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
    [[nodiscard]] double distanceTo(const Position& other) const {
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

    explicit NeuronComponent(PositionPtr  position) : position(std::move(position)) {}
    virtual ~NeuronComponent() = default;

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

    void updateFromAxonBouton(std::shared_ptr<AxonBouton> parentAxonBoutonPointer) { parentAxonBouton = std::move(parentAxonBoutonPointer); }

    [[nodiscard]] std::shared_ptr<AxonBouton> getParentAxonBouton() const { return parentAxonBouton; }

    void UpdateFromDendriteBouton(std::shared_ptr<DendriteBouton> parentDendriteBoutonPointer) { parentDendriteBouton = std::move(parentDendriteBoutonPointer); }

    [[nodiscard]] std::shared_ptr<DendriteBouton> getParentDendriteBouton() const { return parentDendriteBouton; }

private:
    bool instanceInitialised = false;  // Initially, the SynapticGap is not initialised
    bool associated = false;  // Initially, the SynapticGap is not associated with a DendriteBouton
    std::shared_ptr<AxonBouton> parentAxonBouton;
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

private:
    bool instanceInitialised = false;  // Initially, the DendriteBouton is not initialised
    std::shared_ptr<SynapticGap> onwardSynapticGap;
    std::weak_ptr<Neuron> neuron;
    std::shared_ptr<Dendrite> parentDendrite;
};

// Dendrite class
class Dendrite : public std::enable_shared_from_this<Dendrite>, public NeuronComponent<Position>, public NeuronShapeComponent {
public:
    explicit Dendrite(const PositionPtr& position) : NeuronComponent(position), NeuronShapeComponent() {
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

private:
    bool instanceInitialised = false;  // Initially, the Dendrite is not initialised
    std::vector<std::shared_ptr<DendriteBranch>> dendriteBranches;
    std::shared_ptr<DendriteBranch> dendriteBranch;
    std::shared_ptr<DendriteBouton> dendriteBouton;
    std::shared_ptr<DendriteBranch> parentDendriteBranch;
};

// Dendrite branch class
class DendriteBranch : public std::enable_shared_from_this<DendriteBranch>, public NeuronComponent<Position>, public NeuronShapeComponent {
public:
    explicit DendriteBranch(const PositionPtr& position) : NeuronComponent(position), NeuronShapeComponent() {
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
class AxonBouton : public std::enable_shared_from_this<AxonBouton>, public NeuronComponent<Position>, public NeuronShapeComponent {
public:
    explicit AxonBouton(const PositionPtr& position) : NeuronComponent(position),
                                                           NeuronShapeComponent() {
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

private:
    bool instanceInitialised = false;
    std::shared_ptr<SynapticGap> onwardSynapticGap;
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

    void updateFromAxonHillock(std::shared_ptr<AxonHillock> parentAxonHillockPointer) { parentAxonHillock = std::move(parentAxonHillockPointer); }

    [[nodiscard]] std::shared_ptr<AxonHillock> getParentAxonHillock() const { return parentAxonHillock; }

    void updateFromAxonBranch(std::shared_ptr<AxonBranch> parentAxonBranchPointer) { parentAxonBranch = std::move(parentAxonBranchPointer); }

    [[nodiscard]] std::shared_ptr<AxonBranch> getParentAxonBranch() const { return parentAxonBranch; }

private:
    bool instanceInitialised = false;
    std::vector<std::shared_ptr<AxonBranch>> axonBranches;
    std::shared_ptr<AxonBouton> onwardAxonBouton;
    std::shared_ptr<AxonHillock> parentAxonHillock;
    std::shared_ptr<AxonBranch> parentAxonBranch;
};

// Axon branch class
class AxonBranch : public std::enable_shared_from_this<AxonBranch>, public NeuronComponent<Position>, public NeuronShapeComponent {
public:
    explicit AxonBranch(const PositionPtr& position) : NeuronComponent(position), NeuronShapeComponent() {
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
class AxonHillock : public std::enable_shared_from_this<AxonHillock>, public NeuronComponent<Position>, public NeuronShapeComponent {
public:
    explicit AxonHillock(const PositionPtr& position) : NeuronComponent(position), NeuronShapeComponent() {
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

private:
    bool instanceInitialised = false;
    std::shared_ptr<Axon> onwardAxon{};
    std::shared_ptr<Soma> parentSoma;
};

class Soma : public std::enable_shared_from_this<Soma>, public NeuronComponent<Position>, public NeuronShapeComponent {
public:
    explicit Soma(const PositionPtr& position) : NeuronComponent(position), NeuronShapeComponent() {
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

private:
    bool instanceInitialised = false;
    std::vector<std::shared_ptr<SynapticGap>> synapticGaps;
    std::vector<std::shared_ptr<DendriteBranch>> dendriteBranches;
    std::shared_ptr<AxonHillock> onwardAxonHillock{};
    std::shared_ptr<Neuron> parentNeuron{};
};

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
        double offsetX = (std::rand() / double(RAND_MAX)) - 0.15;
        double offsetY = (std::rand() / double(RAND_MAX)) - 0.15;
        double offsetZ = (std::rand() / double(RAND_MAX)) - 0.15;

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


void addDendriteBranchPositionsToPolyData(vtkRenderer* renderer, const std::shared_ptr<DendriteBranch>& dendriteBranch, vtkSmartPointer<vtkPoints> definePoints, vtkSmartPointer<vtkIdList> pointIDs) {
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

void addAxonBranchPositionsToPolyData(vtkRenderer* renderer, const std::shared_ptr<AxonBranch>& axonBranch, vtkSmartPointer<vtkPoints> definePoints, vtkSmartPointer<vtkIdList> pointIDs) {
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
        int points_per_layer = std::stoi(config["points_per_layer"]);
        double proximityThreshold = std::stod(config["proximity_threshold"]);

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
        std::shared_ptr<Neuron> prevNeuron;
        for (int i = 0; i < num_neurons; ++i) {
            auto coords = get_coordinates(i, num_neurons, points_per_layer);
            if ( i > 0 ) {
                prevNeuron = neurons.back();
                shiftX = std::get<0>(coords) + (prevNeuron->getSoma()->getAxonHillock()->getPosition()->x);
                shiftY = std::get<1>(coords) + (prevNeuron->getSoma()->getAxonHillock()->getPosition()->y);
                shiftZ = std::get<2>(coords) + (prevNeuron->getSoma()->getAxonHillock()->getPosition()->z);
            }
            //shiftX = 1.1;
            //shiftY = 1.2;
            //shiftZ = 1.3;

            std::cout << "Creating neuron " << i << " at (" << shiftX << ", " << shiftY << ", " << shiftZ << ")" << std::endl;
            neurons.emplace_back(std::make_shared<Neuron>(std::make_shared<Position>(shiftX, shiftY, shiftZ)));
            neurons.back()->initialise();
            // Sparsely associate neurons
            if (i > 0 && i % 3 > 0) {
                neurons.back()->getSoma()->getAxonHillock()->getAxon()->getAxonBouton()->getSynapticGap()->updatePosition( prevNeuron->getSoma()->getDendriteBranches()[0]->getDendrites()[0]->getDendriteBouton()->getPosition());
                associateSynapticGap(*neurons[i - 1], *neurons[i], proximityThreshold);
            }
        }
        std::cout << "Created " << neurons.size() << " neurons." << std::endl;

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
            std::string query = "INSERT INTO neurons (propagation_rate, neuron_type, axon_length) VALUES (" + std::to_string(propagationRate) + ", 0, 0)";
            txn.exec(query);
            txn.commit();
        } else {
            throw std::runtime_error("The propagation rate is not valid. Skipping database insertion.");
        }

        // Iterate over the neurons and add their positions to the vtkPolyData
        for (const auto& neuron : neurons) {
            const PositionPtr &neuronPosition = neuron->getPosition();

            // Create a vtkIdList to hold the point indices of the line vertices
            vtkSmartPointer<vtkIdList> pointIDs = vtkSmartPointer<vtkIdList>::New();
            pointIDs->InsertNextId(
                    definePoints->InsertNextPoint(neuronPosition->x, neuronPosition->y, neuronPosition->z));

            const PositionPtr &somaPosition = neuron->getSoma()->getPosition();
            // Add the sphere centre point to the vtkPolyData
            vtkSmartPointer<vtkSphereSource> sphere = vtkSmartPointer<vtkSphereSource>::New();
            sphere->SetRadius(1.0); // Adjust the sphere radius as needed
            sphere->SetCenter(somaPosition->x, somaPosition->y, somaPosition->z);
            sphere->SetThetaResolution(32); // Set the sphere resolution
            sphere->SetPhiResolution(16);
            sphere->Update();

            // Add the sphere to the renderer
            vtkSmartPointer<vtkPolyDataMapper> sphereMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
            sphereMapper->SetInputData(sphere->GetOutput());
            vtkSmartPointer<vtkActor> sphereActor = vtkSmartPointer<vtkActor>::New();
            sphereActor->SetMapper(sphereMapper);
            renderer->AddActor(sphereActor);

            // Iterate over the dendrite branches and add their positions to the vtkPolyData
            const std::vector<std::shared_ptr<DendriteBranch>> &dendriteBranches = neuron->getSoma()->getDendriteBranches();
            for (const auto &dendriteBranch: dendriteBranches) {
                addDendriteBranchPositionsToPolyData(renderer, dendriteBranch, definePoints, pointIDs);
            }

            // Add the position of the axon hillock to the vtkPoints and vtkPolyData
            const PositionPtr &axonHillockPosition = neuron->getSoma()->getAxonHillock()->getPosition();
            pointIDs->InsertNextId(definePoints->InsertNextPoint(axonHillockPosition->x, axonHillockPosition->y,
                                                                   axonHillockPosition->z));

            // Add the position of the axon to the vtkPoints and vtkPolyData
            const PositionPtr &axonPosition = neuron->getSoma()->getAxonHillock()->getAxon()->getPosition();
            pointIDs->InsertNextId(definePoints->InsertNextPoint(axonPosition->x, axonPosition->y, axonPosition->z));

            // Add the position of the axon bouton to the vtkPoints and vtkPolyData
            const PositionPtr &axonBoutonPosition = neuron->getSoma()->getAxonHillock()->getAxon()->getAxonBouton()->getPosition();
            pointIDs->InsertNextId(definePoints->InsertNextPoint(axonBoutonPosition->x, axonBoutonPosition->y,
                                                                   axonBoutonPosition->z));

            // Add the position of the synaptic gap to the vtkPoints and vtkPolyData
            addSynapticGapToRenderer(renderer,
                                     neuron->getSoma()->getAxonHillock()->getAxon()->getAxonBouton()->getSynapticGap());

            // Iterate over the axon branches and add their positions to the vtkPolyData
            const std::vector<std::shared_ptr<AxonBranch>> &axonBranches = neuron->getSoma()->getAxonHillock()->getAxon()->getAxonBranches();
            for (const auto &axonBranch: axonBranches) {
                addAxonBranchPositionsToPolyData(renderer, axonBranch, definePoints, pointIDs);
            }

            // Add the line to the cell array
            lines->InsertNextCell(pointIDs);


            // Create a vtkPolyData object and set the points and lines as its data
            vtkSmartPointer<vtkPolyData> polyData = vtkSmartPointer<vtkPolyData>::New();
            polyData->SetPoints(definePoints);

            vtkSmartPointer<vtkDelaunay3D> delaunay = vtkSmartPointer<vtkDelaunay3D>::New();
            delaunay->SetInputData(polyData);
            delaunay->SetAlpha(0.1); // Adjust for mesh density
            //delaunay->Update();

            // Extract the surface of the Delaunay output using vtkGeometryFilter
            //vtkSmartPointer<vtkGeometryFilter> geometryFilter = vtkSmartPointer<vtkGeometryFilter>::New();
            //geometryFilter->SetInputConnection(delaunay->GetOutputPort());
            //geometryFilter->Update();

            // Get the vtkPolyData representing the membrane surface
            //vtkSmartPointer<vtkPolyData> membranePolyData = geometryFilter->GetOutput();

            // Create a vtkPolyDataMapper and set the input as the extracted surface
            vtkSmartPointer<vtkPolyDataMapper> membraneMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
            //membraneMapper->SetInputData(membranePolyData);

            // Create a vtkActor for the membrane and set its mapper
            //vtkSmartPointer<vtkActor> membraneActor = vtkSmartPointer<vtkActor>::New();
            //membraneActor->SetMapper(membraneMapper);
            //membraneActor->GetProperty()->SetColor(0.7, 0.7, 0.7);  // Set color to gray

            // Add the membrane actor to the renderer
            // renderer->AddActor(membraneActor);

            polyData->SetLines(lines);

            // Create a mapper and actor for the neuron positions
            vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
            mapper->SetInputData(polyData);
            vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
            actor->SetMapper(mapper);
            renderer->AddActor(actor);
        }
        // Set up the render window and start the interaction
        renderWindow->SetSize(400, 300);
        renderWindow->Render();
        vtkSmartPointer<vtkWindowToImageFilter> windowToImageFilter = vtkSmartPointer<vtkWindowToImageFilter>::New();
        windowToImageFilter->SetInput(renderWindow);
        windowToImageFilter->Update();

        vtkSmartPointer<vtkJPEGWriter> jpegWriter = vtkSmartPointer<vtkJPEGWriter>::New();
        jpegWriter->SetInputConnection(windowToImageFilter->GetOutputPort());
        jpegWriter->SetQuality(80);  // Adjust the JPEG quality as needed
        jpegWriter->Write();

        vtkImageData* imageData = jpegWriter->GetResult();
        if (!imageData) {
            return "";
        }

        unsigned char* buffer = static_cast<unsigned char*>(imageData->GetScalarPointer());
        size_t bufferSize = imageData->GetScalarSize() * imageData->GetNumberOfPoints();

        std::string base64Image = base64_encode(buffer, bufferSize);

        renderWindowInteractor->Start();

    } catch (const std::exception &e) {
        logger << "Error: " << e.what() << std::endl;
        std::cerr << "Error: " << e.what() << std::endl;
    } catch (...) {
        logger << "Error: Unknown exception occurred." << std::endl;
        std::cerr << "Error: Unknown exception occurred." << std::endl;
    }

    return 0;
}
