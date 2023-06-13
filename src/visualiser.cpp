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
#include "vtkincludes.h"
#include "aarnn.h"

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

int main() {
    // Initialize logger
    Logger logger("errors.log");

    // Initialize VTK
    vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
    vtkSmartPointer<vtkRenderWindow> renderWindow = vtkSmartPointer<vtkRenderWindow>::New();
    renderWindow->AddRenderer(renderer);
    vtkSmartPointer<vtkRenderWindowInteractor> render_window_interactor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
    render_window_interactor->SetRenderWindow(renderWindow);

    // Create a vtkPoints object to hold the neuron positions
    vtkSmartPointer<vtkPoints> definePoints = vtkSmartPointer<vtkPoints>::New();

    // Create a vtkCellArray object to hold the lines
    vtkSmartPointer<vtkCellArray> lines = vtkSmartPointer<vtkCellArray>::New();
    double x;
    double y;
    double z;

    try {
        // Read the database connection configuration and simulation configuration
        std::vector<std::string> config_filenames = {"db_connection.conf", "simulation.conf"};
        auto config = read_config(config_filenames);
        std::string connection_string = build_connection_string(config);

        // Connect to PostgreSQL
        pqxx::connection conn(connection_string);

        // Start a transaction
        pqxx::work txn(conn);

        // Continuously update the visualization
        while (true) {
            // Read data from the database
            std::vector<double> propagationRates;
            // Select neurons
            std::cout << "Selecting neurons..." << std::endl;
            pqxx::result neurons = txn.exec("SELECT neuron_id, x, y, z FROM neurons");

            // Map neuron_id to the index of the point in vtkNeurons
            std::unordered_map<int, int> neuronIndexMap;

            // Create VTK points for neurons
            vtkSmartPointer<vtkPoints> vtkNeurons = vtkSmartPointer<vtkPoints>::New();
            int index = 0;
            for (auto neuron : neurons) {
                int neuron_id = neuron[0].as<int>();
                x = neuron[1].as<double>();
                y = neuron[2].as<double>();
                z = neuron[3].as<double>();
                vtkNeurons->InsertNextPoint(x, y, z);
                neuronIndexMap[neuron_id] = index++;
            }

            // Map soma_id to the index of the point in vtkSomas
            std::unordered_map<int, int> somaIndexMap;

// Select somas and create VTK points
            std::cout << "Selecting somas..." << std::endl;
            pqxx::result somas = txn.exec("SELECT soma_id, neuron_id, x, y, z FROM somas");

// Create a points object and vertex array
            vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
            vtkSmartPointer<vtkCellArray> vertices = vtkSmartPointer<vtkCellArray>::New();

// Add the soma points to the points object
            for (auto soma : somas) {
                x = soma[2].as<double>();
                y = soma[3].as<double>();
                z = soma[4].as<double>();
                points->InsertNextPoint(x, y, z);
                vertices->InsertNextCell(1); // We are inserting only one point per cell
                vertices->InsertCellPoint(points->GetNumberOfPoints() - 1);
            }

// Create a polydata object
            vtkSmartPointer<vtkPolyData> pointPolyData = vtkSmartPointer<vtkPolyData>::New();
            pointPolyData->SetPoints(points);
            pointPolyData->SetVerts(vertices);

// Create the sphere source
            vtkSmartPointer<vtkSphereSource> sphereSource = vtkSmartPointer<vtkSphereSource>::New();
            sphereSource->SetRadius(1.0);
            sphereSource->SetThetaResolution(32);
            sphereSource->SetPhiResolution(16);

// Create the glyph3D filter
            vtkSmartPointer<vtkGlyph3D> glyph3D = vtkSmartPointer<vtkGlyph3D>::New();
            glyph3D->SetSourceConnection(sphereSource->GetOutputPort());
            glyph3D->SetInputData(pointPolyData);
            glyph3D->ScalingOff();
            glyph3D->Update();

// Mapper
            vtkSmartPointer<vtkPolyDataMapper> soma_mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
            soma_mapper->SetInputConnection(glyph3D->GetOutputPort());

// Actor
            vtkSmartPointer<vtkActor> soma_actor = vtkSmartPointer<vtkActor>::New();
            soma_actor->SetMapper(soma_mapper);

// Add the actor to the renderer
            renderer->AddActor(soma_actor);

            // Iterate over the dendrite branches and add their positions to the vtkPolyData
            std::cout << "Selecting dendrite branches..." << std::endl;
            pqxx::result dendritebranches = txn.exec("SELECT dendrite_branch_id, soma_id, x, y, z FROM dendritebranches");
            vtkSmartPointer<vtkPoints> vtkDendriteBranches = vtkSmartPointer<vtkPoints>::New();
            for (auto dendritebranch : dendritebranches) {
                x = dendritebranch[2].as<double>();
                y = dendritebranch[3].as<double>();
                z = dendritebranch[4].as<double>();
                vtkDendriteBranches->InsertNextPoint(x, y, z);
                // Link soma with dendritebranch by index
                int soma_id = dendritebranch[1].as<int>();
                //std::cout << "DendriteBranch belongs to soma at index " << somaIndexMap[soma_id] << std::endl;
            }

            // Map axon_hillock_id to the index of the point in vtkAxonHillocks
            std::unordered_map<int, int> axonHillockIndexMap;

            // Add the position of the axon hillock to the vtkPoints and vtkPolyData
            std::cout << "Selecting axon hillocks..." << std::endl;
            pqxx::result axonhillocks = txn.exec("SELECT axon_hillock_id, soma_id, x, y, z FROM axonhillocks");
            vtkSmartPointer<vtkPoints> vtkAxonHillocks = vtkSmartPointer<vtkPoints>::New();
            for (auto axonhillock : axonhillocks) {
                x = axonhillock[2].as<double>();
                y = axonhillock[3].as<double>();
                z = axonhillock[4].as<double>();
                vtkAxonHillocks->InsertNextPoint(x, y, z);
                // Link soma with axonhillock by index
                int soma_id = axonhillock[1].as<int>();
                //std::cout << "AxonHillock belongs to soma at index " << somaIndexMap[soma_id] << std::endl;
            }

            // Map axon_id to the index of the point in vtkAxons
            std::unordered_map<int, int> axonIndexMap;

            // Add the position of the axon to the vtkPoints and vtkPolyData
            std::cout << "Selecting axons..." << std::endl;
            pqxx::result axons = txn.exec("SELECT axon_id, axon_hillock_id, x, y, z FROM axons");
            vtkSmartPointer<vtkPoints> vtkAxons = vtkSmartPointer<vtkPoints>::New();
            for (auto axon : axons) {
                x = axon[2].as<double>();
                y = axon[3].as<double>();
                z = axon[4].as<double>();
                vtkAxons->InsertNextPoint(x, y, z);
                // Link axonhillock with axon by index
                int axon_hillock_id = axon[1].as<int>();
                //std::cout << "Axon belongs to axon hillock at index " << axonHillockIndexMap[axon_hillock_id] << std::endl;
            }

            // Map axon_bouton_id to the index of the point in vtkAxonBoutons
            std::unordered_map<int, int> axonBoutonIndexMap;

            // Add the position of the axon bouton to the vtkPoints and vtkPolyData
            std::cout << "Selecting axon boutons..." << std::endl;
            pqxx::result axonboutons = txn.exec("SELECT axon_bouton_id, axon_id, x, y, z FROM axonboutons");
            vtkSmartPointer<vtkPoints> vtkAxonBoutons = vtkSmartPointer<vtkPoints>::New();
            for (auto axonbouton : axonboutons) {
                x = axonbouton[2].as<double>();
                y = axonbouton[3].as<double>();
                z = axonbouton[4].as<double>();
                vtkAxonBoutons->InsertNextPoint(x, y, z);
                // Link axon with axonbouton by index
                int axon_id = axonbouton[1].as<int>();
                //std::cout << "Axon bouton belongs to axon at index " << axonIndexMap[axon_id] << std::endl;
            }

            // Add the position of the synaptic gap to the vtkPoints and vtkPolyData
            std::cout << "Selecting synaptic gaps..." << std::endl;
            pqxx::result synapticgaps = txn.exec("SELECT synaptic_gap_id, axon_bouton_id, x, y, z FROM synapticgaps");
            vtkSmartPointer<vtkPoints> vtkSynapticGaps = vtkSmartPointer<vtkPoints>::New();
            for (auto synapticgap : synapticgaps) {
                x = synapticgap[2].as<double>();
                y = synapticgap[3].as<double>();
                z = synapticgap[4].as<double>();
                vtkSynapticGaps->InsertNextPoint(x, y, z);
                // Link axonbouton with synapticgap by index
                int axon_bouton_id = synapticgap[1].as<int>();
                //std::cout << "Synaptic Gap belongs to axon bouton at index " << axonBoutonIndexMap[axon_bouton_id] << std::endl;
            }

            // Iterate over the axon branches and add their positions to the vtkPolyData
            std::cout << "Selecting axon branches..." << std::endl;
            pqxx::result axonbranches = txn.exec("SELECT axon_branch_id, axon_id, x, y, z FROM axonbranches");
            vtkSmartPointer<vtkPoints> vtkAxonBranches = vtkSmartPointer<vtkPoints>::New();
            for (auto axonbranch : axonbranches) {
                x = axonbranch[2].as<double>();
                y = axonbranch[3].as<double>();
                z = axonbranch[4].as<double>();
                vtkAxonBranches->InsertNextPoint(x, y, z);
                // Link axon with axonbranches by index
                int axon_id = axonbranch[1].as<int>();
                //std::cout << "Axon branch belongs to axon at index " << axonIndexMap[axon_id] << std::endl;
            }

            // Create a polydata object
            vtkSmartPointer<vtkPolyData> polyData = vtkSmartPointer<vtkPolyData>::New();
            polyData->SetPoints(vtkNeurons);

            // Create a mapper and actor
            vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
            mapper->SetInputData(polyData);
            vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
            actor->SetMapper(mapper);

            // Create a renderer, render window, and interactor
            renderWindow->AddRenderer(renderer);
            vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
            renderWindowInteractor->SetRenderWindow(renderWindow);

            // Add the actor to the scene
            renderer->AddActor(actor);
            renderer->SetBackground(0, 0, 0); // Background color

            // Render and interact
            renderWindow->Render();
            renderWindowInteractor->Start();
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
