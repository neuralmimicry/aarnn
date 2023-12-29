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
#include <condition_variable>
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
#include <fftw3.h>
#include <portaudio.h>
#include <pulse/pulseaudio.h>
#include <pulse/simple.h>
#include <pulse/error.h>
#include <pulse/proplist.h>
#include "boostincludes.h"

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

extern "C" {
#include <libavcodec/avcodec.h>
}

#include "../include/vtkincludes.h"

#include "../include/DendriteBranch.h"
#include "../include/AxonHillock.h"
#include "../include/PulseAudioMic.h"
#include "../include/Logger.h"
#include "../include/nvTimerCallback.h"
#include "../include/avTimerCallback.h"
#include "../include/NeuronParameters.h"
#include "../include/Neuron.h"
#include "../include/Axon.h"

#define PA_SAMPLE_TYPE      paFloat32

std::mt19937 gen(12345); // Always generates the same sequence of numbers
std::uniform_real_distribution<> distr(-0.15, 1.0-0.15);

// Global variables
std::atomic<double> totalPropagationRate(0.0);
std::mutex mtx;

const int SAMPLE_RATE = 16000;
const int FRAMES_PER_BUFFER = 256;
typedef float SAMPLE;
static int gNumNoInputs = 0;

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
    // Function implementation
    // ...
}
std::map<std::string, std::string> read_config(const std::vector<std::string> &filenames) {
 */

template <typename T>
std::map<std::string, std::string> read_config(const std::vector<T>& filenames) {

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

std::atomic<bool> running(true);

void checkForQuit() {
    struct pollfd fds[1];
    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;

    while(running) {
        int ret = poll(fds, 1, 1000); // 1000ms timeout

        if(ret > 0) {
            char key;
            read(STDIN_FILENO, &key, 1);
            if(key == 'q') {
                running = false;
                break;
            }
        } else if(ret < 0) {
            // Handle error
        }

        // Else timeout occurred, just loop back and poll again
    }
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

template <typename Func>
void logExecutionTime(Func function, const std::string& functionName) {
    auto start = std::chrono::high_resolution_clock::now();

    function();

    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::ofstream logFile("execution_times.log", std::ios_base::app); // Open the log file in append mode
    logFile << functionName << " execution time: " << duration.count() << " microseconds\n";
    logFile.close();
}


using PositionPtr = std::shared_ptr<Position>;


static int portaudioMicCallBack(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void *userData)
{
    std::cout << "MicCallBack" << std::endl;
    // Read data from the stream.
    const SAMPLE *in = (const SAMPLE *) inputBuffer;
    SAMPLE *out = (SAMPLE *) outputBuffer;
    (void) timeInfo; /* Prevent unused variable warnings. */
    (void) statusFlags;
    (void) userData;

    if (inputBuffer == nullptr) {
        for (unsigned int i = 0; i < framesPerBuffer; i++) {
            *out++ = 0;  /* left - silent */
            *out++ = 0;  /* right - silent */
        }
        gNumNoInputs += 1;
    } else {
        for (unsigned int i = 0; i < framesPerBuffer; i++) {
            // Here you might want to capture audio data
            //capturedAudio.push_back(in[i]);
        }
    }
    return paContinue;
}

void Dendrite::addBranch(std::shared_ptr<DendriteBranch> branch) {
    auto coords = get_coordinates(int (dendriteBranches.size() + 1), int(dendriteBranches.size() + 1), int(5));
    PositionPtr currentPosition = branch->getPosition();
    auto x = double(std::get<0>(coords)) + currentPosition->x;
    auto y = double(std::get<1>(coords)) + currentPosition->y;
    auto z = double(std::get<2>(coords)) + currentPosition->z;
    currentPosition->x = x;
    currentPosition->y = y;
    currentPosition->z = z;
    dendriteBranches.emplace_back(std::move(branch));
}

void Axon::addBranch(std::shared_ptr<AxonBranch> branch) {
    auto coords = get_coordinates(int (axonBranches.size() + 1), int(axonBranches.size() + 1), int(5));
    PositionPtr currentPosition = branch->getPosition();
    auto x = double(std::get<0>(coords)) + currentPosition->x;
    auto y = double(std::get<1>(coords)) + currentPosition->y;
    auto z = double(std::get<2>(coords)) + currentPosition->z;
    currentPosition->x = x;
    currentPosition->y = y;
    currentPosition->z = z;
    axonBranches.emplace_back(std::move(branch));
}

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
                std::cout << "Checking gap " << gap->getPosition() << " with dendrite bouton " << dendriteBouton->getPosition() << std::endl;
                // If the distance between the bouton and the dendriteBouton is below the proximity threshold
                if (gap->getPosition()->distanceTo(*(dendriteBouton->getPosition())) < proximityThreshold) {
                    // Associate the synaptic gap with the dendriteBouton
                    dendriteBouton->connectSynapticGap(gap);
                    // Set the SynapticGap as associated
                    gap->setAsAssociated();
                    std::cout << "Associated gap " << gap->getPosition() << " with dendrite bouton " << dendriteBouton->getPosition() << std::endl;
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
                std::cout << "Checking gap " << gap->getPosition() << " with dendrite bouton " << dendriteBouton->getPosition() << std::endl;

                // If the distance between the gap and the dendriteBouton is below the proximity threshold
                if (gap->getPosition()->distanceTo(*(dendriteBouton->getPosition())) < proximityThreshold) {
                    // Associate the synaptic gap with the dendriteBouton
                    dendriteBouton->connectSynapticGap(gap);
                    // Set the SynapticGap as associated
                    gap->setAsAssociated();
                    std::cout << "Associated gap " << gap->getPosition() << " with dendrite bouton " << dendriteBouton->getPosition() << std::endl;
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
            "DROP TABLE IF EXISTS neurons, somas, axonhillocks, axons, axons_hillock, axonboutons, synapticgaps, dendritebranches_soma, dendritebranches, dendrites, dendriteboutons, axonbranches CASCADE; COMMIT;"
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

                "CREATE TABLE axons_hillock ("
                "axon_id SERIAL PRIMARY KEY,"
                "axon_hillock_id INTEGER,"
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

                "ALTER TABLE axons_hillock "
                "ADD CONSTRAINT fk_axons_hillock_axon_id "
                "FOREIGN KEY (axon_id) REFERENCES axons(axon_id);"

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

                "CREATE TABLE axonbranches ("
                "axon_branch_id SERIAL PRIMARY KEY,"
                "axon_id INTEGER REFERENCES axons(axon_id),"
                "x REAL NOT NULL,"
                "y REAL NOT NULL,"
                "z REAL NOT NULL"
                ");"

                "CREATE TABLE dendritebranches_soma ("
                "dendrite_branch_id SERIAL PRIMARY KEY,"
                "soma_id INTEGER REFERENCES somas(soma_id),"
                "x REAL NOT NULL,"
                "y REAL NOT NULL,"
                "z REAL NOT NULL"
                ");"

                "CREATE TABLE dendrites ("
                "dendrite_id SERIAL PRIMARY KEY,"
                "dendrite_branch_id INTEGER,"
                "x REAL NOT NULL,"
                "y REAL NOT NULL,"
                "z REAL NOT NULL"
                ");"

                "CREATE TABLE dendritebranches ("
                "dendrite_branch_id SERIAL PRIMARY KEY,"
                "dendrite_id INTEGER REFERENCES dendrites(dendrite_id),"
                "x REAL NOT NULL,"
                "y REAL NOT NULL,"
                "z REAL NOT NULL"
                ");"

                "ALTER TABLE dendritebranches "
                "ADD CONSTRAINT fk_dendritebranches_dendrite_id "
                "FOREIGN KEY (dendrite_id) REFERENCES dendrites(dendrite_id);"

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
                                 const vtkSmartPointer<vtkPoints>& points, const vtkSmartPointer<vtkIdList>& pointIds) {
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
                             const vtkSmartPointer<vtkPoints>& points, const vtkSmartPointer<vtkIdList>& pointIds) {
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

void insertDendriteBranches(pqxx::transaction_base& txn, const std::shared_ptr<DendriteBranch>& dendriteBranch, int& dendrite_branch_id, int& dendrite_id, int& dendrite_bouton_id, int& soma_id) {
    std::string query;

    if (dendriteBranch->getParentSoma()) {
        query = "INSERT INTO dendritebranches_soma (dendrite_branch_id, soma_id, x, y, z) VALUES (" +
                std::to_string(dendrite_branch_id) + ", " + std::to_string(soma_id) + ", " +
                std::to_string(dendriteBranch->getPosition()->x) + ", " +
                std::to_string(dendriteBranch->getPosition()->y) + ", " +
                std::to_string(dendriteBranch->getPosition()->z) + ")";
        txn.exec(query);
    }
    else {
        query = "INSERT INTO dendritebranches (dendrite_branch_id, dendrite_id, x, y, z) VALUES (" +
                std::to_string(dendrite_branch_id) + ", " + std::to_string(dendrite_id) + ", " +
                std::to_string(dendriteBranch->getPosition()->x) + ", " +
                std::to_string(dendriteBranch->getPosition()->y) + ", " +
                std::to_string(dendriteBranch->getPosition()->z) + ")";
        txn.exec(query);
    }

    for (auto& dendrite : dendriteBranch->getDendrites()) {
        query = "INSERT INTO dendrites (dendrite_id, dendrite_branch_id, x, y, z) VALUES (" +
                std::to_string(dendrite_id) + ", " + std::to_string(dendrite_branch_id) + ", " +
                std::to_string(dendrite->getPosition()->x) + ", " +
                std::to_string(dendrite->getPosition()->y) + ", " +
                std::to_string(dendrite->getPosition()->z) + ")";
        txn.exec(query);

        query = "INSERT INTO dendriteboutons (dendrite_bouton_id, dendrite_id, x, y, z) VALUES (" +
                std::to_string(dendrite_bouton_id) + ", " + std::to_string(dendrite_id) + ", " +
                std::to_string(dendrite->getDendriteBouton()->getPosition()->x) + ", " +
                std::to_string(dendrite->getDendriteBouton()->getPosition()->y) + ", " +
                std::to_string(dendrite->getDendriteBouton()->getPosition()->z) + ")";
        txn.exec(query);

        // Increment dendrite_id here after the query
        dendrite_id++;

        // Increment dendrite_bouton_id here after the query
        dendrite_bouton_id++;

        for (auto& innerDendriteBranch : dendrite->getDendriteBranches()) {
            // Increment dendrite_branch_id here after the query
            dendrite_branch_id++;
            insertDendriteBranches(txn, innerDendriteBranch, dendrite_branch_id, dendrite_id, dendrite_bouton_id, soma_id);
        }
    }
    // Increment dendrite_branch_id here after the query
    dendrite_branch_id++;
}

void insertAxonBranches(pqxx::transaction_base& txn, const std::shared_ptr<AxonBranch>& axonBranch, int& axon_branch_id, int& axon_id, int& axon_bouton_id, int& synaptic_gap_id, int& axon_hillock_id) {
    std::string query;

    query = "INSERT INTO axonbranches (axon_branch_id, axon_id, x, y, z) VALUES (" +
            std::to_string(axon_branch_id) + ", " + std::to_string(axon_id) + ", " +
            std::to_string(axonBranch->getPosition()->x) + ", " +
            std::to_string(axonBranch->getPosition()->y) + ", " +
            std::to_string(axonBranch->getPosition()->z) + ")";
    txn.exec(query);

    for (auto& axon : axonBranch->getAxons()) {
        // Increment axon_id here after the query
        axon_id++;
            query = "INSERT INTO axons (axon_id, axon_branch_id, x, y, z) VALUES (" +
                    std::to_string(axon_id) + ", " + std::to_string(axon_branch_id) + ", " +
                    std::to_string(axon->getPosition()->x) + ", " +
                    std::to_string(axon->getPosition()->y) + ", " +
                    std::to_string(axon->getPosition()->z) + ")";
        txn.exec(query);

        query = "INSERT INTO axonboutons (axon_bouton_id, axon_id, x, y, z) VALUES (" +
                std::to_string(axon_bouton_id) + ", " + std::to_string(axon_id) + ", " +
                std::to_string(axon->getAxonBouton()->getPosition()->x) + ", " +
                std::to_string(axon->getAxonBouton()->getPosition()->y) + ", " +
                std::to_string(axon->getAxonBouton()->getPosition()->z) + ")";
        txn.exec(query);

        query = "INSERT INTO synapticgaps (synaptic_gap_id, axon_bouton_id, x, y, z) VALUES (" +
                std::to_string(synaptic_gap_id) + ", " + std::to_string(axon_bouton_id) + ", " +
                std::to_string(axon->getAxonBouton()->getSynapticGap()->getPosition()->x) + ", " +
                std::to_string(axon->getAxonBouton()->getSynapticGap()->getPosition()->y) + ", " +
                std::to_string(axon->getAxonBouton()->getSynapticGap()->getPosition()->z) + ")";
        txn.exec(query);

        // Increment axon_bouton_id here after the query
        axon_bouton_id++;
        // Increment synaptic_gap_id here after the query
        synaptic_gap_id++;

        for (auto& innerAxonBranch : axon->getAxonBranches()) {
            // Increment axon_branch_id here after the query
            axon_branch_id++;
            insertAxonBranches(txn, innerAxonBranch, axon_branch_id, axon_id, axon_bouton_id, synaptic_gap_id, axon_hillock_id);
        }
    }
}

void runInteractor(std::vector<std::shared_ptr<Neuron>>& neurons, std::mutex& neuron_mutex, ThreadSafeQueue<std::vector<std::tuple<double, double>>>& audioQueue, int whichCallBack)
{
    // Initialize VTK
    //vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
    //vtkSmartPointer<vtkCellArray> lines = vtkSmartPointer<vtkCellArray>::New();
    //vtkSmartPointer<vtkPolyData> polyData = vtkSmartPointer<vtkPolyData>::New();
    //vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    //polyData->SetPoints(points);
    //polyData->SetLines(lines);
    //mapper->SetInputData(polyData);
    //vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    //vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
    //vtkSmartPointer<vtkRenderWindow> renderWindow = vtkSmartPointer<vtkRenderWindow>::New();
    //actor->SetMapper(mapper);
    //renderWindow->AddRenderer(renderer);

    if (whichCallBack == 0) {

        vtkSmartPointer<nvTimerCallback> nvCB = vtkSmartPointer<nvTimerCallback>::New();
        nvCB->setNeurons(neurons);
        nvCB->setMutex(neuron_mutex);
        //nvCB->setPoints(points);
        //nvCB->setLines(lines);
        //nvCB->setPolyData(polyData);
        //nvCB->setActor(actor);
        //nvCB->setMapper(mapper);
        //nvCB->setRenderer(renderer);
        nvCB->nvRenderWindow->AddRenderer(nvCB->nvRenderer);

        // Custom main loop
        while (true) { // or a more appropriate loop condition
            auto executeFunc = [nvCB] () mutable { nvCB->Execute(nullptr, vtkCommand::TimerEvent, nullptr); };
            logExecutionTime(executeFunc, "nvCB->Execute");
            nvCB->nvRenderWindow->Render();
            std::cout << "x";
            //std::this_thread::sleep_for(std::chrono::milliseconds(501));
        }
    }
    if (whichCallBack == 1) {
        vtkSmartPointer<avTimerCallback> avCB = vtkSmartPointer<avTimerCallback>::New();
        avCB->setAudio(audioQueue);
        //avCB->setPoints(points);
        //avCB->setLines(lines);
        //avCB->setPolyData(polyData);
        //avCB->setActor(actor);
        //avCB->setMapper(mapper);
        //avCB->setRenderer(renderer);
        avCB->avRenderWindow->AddRenderer(avCB->avRenderer);

        // Custom main loop
        while (true) { // or a more appropriate loop condition
            auto executeFunc = [avCB] () mutable { avCB->Execute(nullptr, vtkCommand::TimerEvent, nullptr); };
            logExecutionTime(executeFunc, "avCB->Execute");
            avCB->avRenderWindow->AddRenderer(avCB->avRenderer);
            avCB->avRenderWindow->Render();
            std::cout << "X";
            //std::this_thread::sleep_for(std::chrono::milliseconds(99));
        }
    }
}

/**
 * @brief Main function to initialize, run, and finalize the neuron network simulation.
 * @return int Status code (0 for success, non-zero for failures).
 */
int main();
int main() {
    Logger logger("errors.log");

    std::string input = "Hello, World!";
    std::string encoded = base64_encode(reinterpret_cast<const unsigned char*>(input.c_str()), input.length());

    std::cout << "Base64 Encoded: " << encoded << std::endl;
    std::string query;

    ThreadSafeQueue<std::vector<std::tuple<double, double>>> audioQueue;
    ThreadSafeQueue<std::vector<std::tuple<double, double>>> emptyAudioQueue;

    std::shared_ptr<PulseAudioMic> mic = std::make_shared<PulseAudioMic>(audioQueue);
    std::thread micThread(&PulseAudioMic::micRun, mic);

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
    std::mutex neuron_mutex;
    std::mutex empty_neuron_mutex;
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
            shiftX = std::get<0>(coords) + (0.1 * i) + sin((3.14 / 180) * (i * 10)) * 0.1;
            shiftY = std::get<1>(coords) + (0.1 * i) + cos((3.14 / 180) * (i * 10)) * 0.1;
            shiftZ = std::get<2>(coords) + (0.1 * i) + sin((3.14 / 180) * (i * 10)) * 0.1;
        }

        // std::cout << "Creating neuron " << i << " at (" << shiftX << ", " << shiftY << ", " << shiftZ << ")" << std::endl;
        neurons.emplace_back(std::make_shared<Neuron>(std::make_shared<Position>(shiftX, shiftY, shiftZ)));
        neurons.back()->initialise();
        // Sparsely associate neurons
        if (i > 0 && i % 3 == 0) {
            //std::cout << "Associating neuron " << i << " with neuron " << i - 1 << std::endl;
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
            if (i > 0 && i % 7 == 0) {
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
            if (i > 0 && i % 11 == 0) {
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
            if (i > 0 && i % 13 == 0) {
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
        if (i > 0 && i % 17 == 0 && !neurons[int(i + num_vocels)]->getSoma()->getAxonHillock()->getAxon()->getAxonBouton()->getSynapticGap()->isAssociated() ) {
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
        //std::cout << query << std::endl;
        txn.exec(query);
        query = "INSERT INTO somas (soma_id, neuron_id, x, y, z) VALUES (" + std::to_string(soma_id) + ", " + std::to_string(neuron_id) + ", " + std::to_string(neuron->getSoma()->getPosition()->x) + ", " + std::to_string(neuron->getSoma()->getPosition()->y) + ", " + std::to_string(neuron->getSoma()->getPosition()->z) + ")";
        //std::cout << query << std::endl;
        txn.exec(query);
        query = "INSERT INTO axonhillocks (axon_hillock_id, soma_id, x, y, z) VALUES (" + std::to_string(axon_hillock_id) + ", " + std::to_string(soma_id) + ", " + std::to_string(neuron->getSoma()->getAxonHillock()->getPosition()->x) + ", " + std::to_string(neuron->getSoma()->getAxonHillock()->getPosition()->y) + ", " + std::to_string(neuron->getSoma()->getAxonHillock()->getPosition()->z) + ")";
        //std::cout << query << std::endl;
        txn.exec(query);
        query = "INSERT INTO axons (axon_id, axon_hillock_id, x, y, z) VALUES (" + std::to_string(axon_id) + ", " + std::to_string(axon_hillock_id) + ", " + std::to_string(neuron->getSoma()->getAxonHillock()->getAxon()->getPosition()->x) + ", " + std::to_string(neuron->getSoma()->getAxonHillock()->getAxon()->getPosition()->y) + ", " + std::to_string(neuron->getSoma()->getAxonHillock()->getAxon()->getPosition()->z) + ")";
        //std::cout << query << std::endl;
        txn.exec(query);
        query = "INSERT INTO axonboutons (axon_bouton_id, axon_id, x, y, z) VALUES (" + std::to_string(axon_bouton_id) + ", " + std::to_string(axon_id) + ", " + std::to_string(neuron->getSoma()->getAxonHillock()->getAxon()->getAxonBouton()->getPosition()->x) + ", " + std::to_string(neuron->getSoma()->getAxonHillock()->getAxon()->getAxonBouton()->getPosition()->y) + ", " + std::to_string(neuron->getSoma()->getAxonHillock()->getAxon()->getAxonBouton()->getPosition()->z) + ")";
        //std::cout << query << std::endl;
        txn.exec(query);
        query = "INSERT INTO synapticgaps (synaptic_gap_id, axon_bouton_id, x, y, z) VALUES (" + std::to_string(synaptic_gap_id) + ", " + std::to_string(axon_bouton_id) + ", " + std::to_string(neuron->getSoma()->getAxonHillock()->getAxon()->getAxonBouton()->getSynapticGap()->getPosition()->x) + ", " + std::to_string(neuron->getSoma()->getAxonHillock()->getAxon()->getAxonBouton()->getSynapticGap()->getPosition()->y) + ", " + std::to_string(neuron->getSoma()->getAxonHillock()->getAxon()->getAxonBouton()->getSynapticGap()->getPosition()->z) + ")";
        //std::cout << query << std::endl;
        txn.exec(query);
        axon_bouton_id++;
        synaptic_gap_id++;

        if (neuron &&
            neuron->getSoma() &&
            neuron->getSoma()->getAxonHillock() &&
            neuron->getSoma()->getAxonHillock()->getAxon() &&
            !neuron->getSoma()->getAxonHillock()->getAxon()->getAxonBranches().empty() &&
            neuron->getSoma()->getAxonHillock()->getAxon()->getAxonBranches()[0] ) {

            for (auto& axonBranch : neuron->getSoma()->getAxonHillock()->getAxon()->getAxonBranches()) {
                insertAxonBranches(txn, axonBranch, axon_branch_id, axon_id, axon_bouton_id, synaptic_gap_id, axon_hillock_id);
                axon_branch_id++;
            }
        }

        for (auto& dendriteBranch : neuron->getSoma()->getDendriteBranches()) {
            insertDendriteBranches(txn, dendriteBranch, dendrite_branch_id, dendrite_id, dendrite_bouton_id, soma_id);
        }

        axon_id++;
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

    std::vector<std::shared_ptr<Neuron>> emptyNeurons;
    // Set up and start the interactors in separate threads
    std::thread nvThread(runInteractor, std::ref(neurons), std::ref(neuron_mutex), std::ref(emptyAudioQueue), 0);
    std::thread avThread(runInteractor, std::ref(emptyNeurons), std::ref(empty_neuron_mutex), std::ref(audioQueue), 1);
    std::thread inputThread(checkForQuit);

    while(running) {
        std::cout << "v";
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // sleep for 100 ms
    }

    // Wait for both threads to finish
    inputThread.join();
    nvThread.join();
    avThread.join();
    mic->micStop();
    micThread.join();

    return 0;
}
