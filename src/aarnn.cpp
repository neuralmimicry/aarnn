/**
 * @file  aarnn.cpp
 * @brief This file contains the implementation of a neuronal network simulation.
 * @author Paul B. Isaac's
 * @version 0.1
 * @date  12-May-2023
 */
#include "DB.h"
#include "boostincludes.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdlib>
#include <cstring>
#include <fftw3.h>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <portaudio.h>
#include <pulse/error.h>
#include <pulse/proplist.h>
#include <pulse/pulseaudio.h>
#include <pulse/simple.h>
#include <random>
#include <set>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <string>
#include <thread>
#include <tuple>
#include <utility>
#include <vector>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

extern "C"
{
#include <libavcodec/avcodec.h>
}

#include "Axon.h"
#include "AxonBouton.h"
#include "AxonHillock.h"
#include "DendriteBranch.h"
#include "Neuron.h"
#include "NeuronParameters.h"
#include "PulseAudioMic.h"
#include "SensoryReceptor.h"
#include "SynapticGap.h"
#include "avTimerCallback.h"
#include "config.h"
#include "nvTimerCallback.h"
#include "utils.h"
#include "vtkincludes.h"

#define PA_SAMPLE_TYPE paFloat32

// Global variables
std::atomic<double> totalPropagationRate(0.0);
std::mutex          mtx;

// websocketpp typedefs
typedef websocketpp::server<websocketpp::config::asio> Server;
typedef websocketpp::connection_hdl                    ConnectionHandle;

std::atomic<bool> running{true};

void checkForQuit()
{
    struct pollfd fds[1];
    fds[0].fd     = STDIN_FILENO;
    fds[0].events = POLLIN;

    while(running)
    {
        int ret = poll(fds, 1, 1000);  // 1000ms timeout

        if(ret > 0)
        {
            char key;
            read(STDIN_FILENO, &key, 1);
            if(key == 'q')
            {
                running = false;
                break;
            }
        }
        else if(ret < 0)
        {
            // Handle error
        }

        // Else timeout occurred, just loop back and poll again
    }
}

static int portaudioMicCallBack(const void*                     inputBuffer,
                                void*                           outputBuffer,
                                unsigned long                   framesPerBuffer,
                                const PaStreamCallbackTimeInfo* timeInfo,
                                PaStreamCallbackFlags           statusFlags,
                                void*                           userData)
{
    spdlog::info("MicCallBack");
    // Read data from the stream.
    const SAMPLE* in  = (const SAMPLE*)inputBuffer;
    SAMPLE*       out = (SAMPLE*)outputBuffer;
    (void)timeInfo; /* Prevent unused variable warnings. */
    (void)statusFlags;
    (void)userData;

    if(inputBuffer == nullptr)
    {
        for(unsigned int i = 0; i < framesPerBuffer; i++)
        {
            *out++ = 0; /* left - silent */
            *out++ = 0; /* right - silent */
        }
        gNumNoInputs += 1;
    }
    else
    {
        for(unsigned int i = 0; i < framesPerBuffer; i++)
        {
            // Here you might want to capture audio data
            // capturedAudio.push_back(in[i]);
        }
    }
    return paContinue;
}

void Dendrite::addBranch(std::shared_ptr<DendriteBranch> branch)
{
    auto        coords = get_coordinates(int(dendriteBranches.size() + 1), int(dendriteBranches.size() + 1), int(5));
    PositionPtr currentPosition = branch->getPosition();
    auto        x               = double(std::get<0>(coords)) + currentPosition->x;
    auto        y               = double(std::get<1>(coords)) + currentPosition->y;
    auto        z               = double(std::get<2>(coords)) + currentPosition->z;
    currentPosition->x          = x;
    currentPosition->y          = y;
    currentPosition->z          = z;
    dendriteBranches.emplace_back(std::move(branch));
}

/**
 * @brief Compute the propagation rate of a neuron.
 * @param neuron Pointer to the Neuron object for which the propagation rate is to be calculated.
 */
void computePropagationRate(const std::shared_ptr<Neuron>& neuron)
{
    // Get the propagation rate of the neuron from the DendriteBouton to the SynapticGap
    double propagationRate = neuron->getSoma()->getPropagationRate();

    // Lock the mutex to safely update the totalPropagationRate
    {
        std::lock_guard<std::mutex> lock(mtx);
        totalPropagationRate = totalPropagationRate + propagationRate;
    }
}

void associateSynapticGap(Neuron& neuron1, Neuron& neuron2, double proximityThreshold)
{
    // Iterate over all SynapticGaps in neuron1
    for(auto& gap: neuron1.getSynapticGapsAxon())
    {
        // If the bouton has a synaptic gap, and it is not associated yet
        if(gap && !gap->isAssociated())
        {
            // Iterate over all DendriteBoutons in neuron2
            for(auto& dendriteBouton: neuron2.getDendriteBoutons())
            {
                spdlog::info("Checking gap {} with dendrite bouton {}",
                             gap->positionAsString(),
                             dendriteBouton->positionAsString());
                // If the distance between the bouton and the dendriteBouton is below the proximity threshold
                if(gap->getPosition()->distanceTo(*(dendriteBouton->getPosition())) < proximityThreshold)
                {
                    // Associate the synaptic gap with the dendriteBouton
                    dendriteBouton->connectSynapticGap(gap);
                    // Set the SynapticGap as associated
                    gap->setAsAssociated();
                    spdlog::info("Associated gap {} with dendrite bouton {}",
                                 gap->positionAsString(),
                                 dendriteBouton->positionAsString());
                    // No need to check other DendriteBoutons once the gap is associated
                    break;
                }
            }
        }
    }
}

void associateSynapticGap(SensoryReceptor& receptor, Neuron& neuron, double proximityThreshold)
{
    // Iterate over all SynapticGaps of receptor
    for(auto& gap: receptor.getSynapticGaps())
    {
        // If the bouton has a synaptic gap, and it is not associated yet
        if(gap && !gap->isAssociated())
        {
            // Iterate over all DendriteBoutons in neuron
            for(auto& dendriteBouton: neuron.getDendriteBoutons())
            {
                spdlog::info("Checking gap {} with dendrite bouton {}",
                             gap->positionAsString(),
                             dendriteBouton->positionAsString());

                // If the distance between the gap and the dendriteBouton is below the proximity threshold
                if(gap->getPosition()->distanceTo(*(dendriteBouton->getPosition())) < proximityThreshold)
                {
                    // Associate the synaptic gap with the dendriteBouton
                    dendriteBouton->connectSynapticGap(gap);
                    // Set the SynapticGap as associated
                    gap->setAsAssociated();
                    spdlog::info("Associated gap {} with dendrite bouton {}",
                                 gap->positionAsString(),
                                 dendriteBouton->positionAsString());
                    // No need to check other DendriteBoutons once the gap is associated
                    break;
                }
            }
        }
    }
}

// const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
// std::string base64_encode(const unsigned char* data, size_t length)
// {
//     std::string   base64;
//     int           i = 0;
//     int           j = 0;
//     unsigned char char_array_3[3];
//     unsigned char char_array_4[4];
//
//     while(length--)
//     {
//         char_array_3[i++] = *(data++);
//         if(i == 3)
//         {
//             char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
//             char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
//             char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
//             char_array_4[3] = char_array_3[2] & 0x3f;
//
//             for(i = 0; i < 4; i++)
//             {
//                 base64 += base64_chars[char_array_4[i]];
//             }
//             i = 0;
//         }
//     }
//
//     if(i)
//     {
//         for(j = i; j < 3; j++)
//         {
//             char_array_3[j] = '\0';
//         }
//
//         char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
//         char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
//         char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
//         char_array_4[3] = char_array_3[2] & 0x3f;
//
//         for(j = 0; j < i + 1; j++)
//         {
//             base64 += base64_chars[char_array_4[j]];
//         }
//
//         while((i++ < 3))
//         {
//             base64 += '=';
//         }
//     }
//
//     return base64;
// }

bool convertStringToBool(const std::string& value)
{
    // Convert to lowercase for case-insensitive comparison
    std::string lowerValue = value;
    std::transform(lowerValue.begin(), lowerValue.end(), lowerValue.begin(), ::tolower);

    if(lowerValue == "true" || lowerValue == "yes" || lowerValue == "1")
    {
        return true;
    }
    else if(lowerValue == "false" || lowerValue == "no" || lowerValue == "0")
    {
        return false;
    }
    else
    {
        // Handle error or throw an exception
        std::cerr << "Invalid boolean string representation: " << value << std::endl;
        return false;
    }
}

void insertDendriteBranches(DB                                     database,
                            const std::shared_ptr<DendriteBranch>& dendriteBranch,
                            int&                                   dendrite_branch_id,
                            int&                                   dendrite_id,
                            int&                                   dendrite_bouton_id,
                            int&                                   soma_id)
{
    std::string query;

    if(dendriteBranch->getParentSoma())
    {
        query = "INSERT INTO dendritebranches_soma (dendrite_branch_id, soma_id, x, y, z) VALUES ("
                + std::to_string(dendrite_branch_id) + ", " + std::to_string(soma_id) + ", "
                + std::to_string(dendriteBranch->getPosition()->x) + ", "
                + std::to_string(dendriteBranch->getPosition()->y) + ", "
                + std::to_string(dendriteBranch->getPosition()->z) + ")";
        database.exec(query);
    }
    else
    {
        query = "INSERT INTO dendritebranches (dendrite_branch_id, dendrite_id, x, y, z) VALUES ("
                + std::to_string(dendrite_branch_id) + ", " + std::to_string(dendrite_id) + ", "
                + std::to_string(dendriteBranch->getPosition()->x) + ", "
                + std::to_string(dendriteBranch->getPosition()->y) + ", "
                + std::to_string(dendriteBranch->getPosition()->z) + ")";
        database.exec(query);
    }

    for(auto& dendrite: dendriteBranch->getDendrites())
    {
        query = "INSERT INTO dendrites (dendrite_id, dendrite_branch_id, x, y, z) VALUES ("
                + std::to_string(dendrite_id) + ", " + std::to_string(dendrite_branch_id) + ", "
                + std::to_string(dendrite->getPosition()->x) + ", " + std::to_string(dendrite->getPosition()->y) + ", "
                + std::to_string(dendrite->getPosition()->z) + ")";
        database.exec(query);

        query = "INSERT INTO dendriteboutons (dendrite_bouton_id, dendrite_id, x, y, z) VALUES ("
                + std::to_string(dendrite_bouton_id) + ", " + std::to_string(dendrite_id) + ", "
                + std::to_string(dendrite->getDendriteBouton()->getPosition()->x) + ", "
                + std::to_string(dendrite->getDendriteBouton()->getPosition()->y) + ", "
                + std::to_string(dendrite->getDendriteBouton()->getPosition()->z) + ")";
        database.exec(query);

        // Increment dendrite_id here after the query
        dendrite_id++;

        // Increment dendrite_bouton_id here after the query
        dendrite_bouton_id++;

        for(auto& innerDendriteBranch: dendrite->getDendriteBranches())
        {
            // Increment dendrite_branch_id here after the query
            dendrite_branch_id++;
            insertDendriteBranches(database,
                                   innerDendriteBranch,
                                   dendrite_branch_id,
                                   dendrite_id,
                                   dendrite_bouton_id,
                                   soma_id);
        }
    }
    // Increment dendrite_branch_id here after the query
    dendrite_branch_id++;
}

void insertAxonBranches(DB&                                database,
                        const std::shared_ptr<AxonBranch>& axonBranch,
                        int&                               axon_branch_id,
                        int&                               axon_id,
                        int&                               axon_bouton_id,
                        int&                               synaptic_gap_id,
                        int&                               axon_hillock_id)
{
    std::string query;

    query = "INSERT INTO axonbranches (axon_branch_id, axon_id, x, y, z) VALUES (" + std::to_string(axon_branch_id)
            + ", " + std::to_string(axon_id) + ", " + std::to_string(axonBranch->getPosition()->x) + ", "
            + std::to_string(axonBranch->getPosition()->y) + ", " + std::to_string(axonBranch->getPosition()->z) + ")";
    database.exec(query);

    for(auto& axon: axonBranch->getAxons())
    {
        // Increment axon_id here after the query
        axon_id++;
        query = "INSERT INTO axons (axon_id, axon_branch_id, x, y, z) VALUES (" + std::to_string(axon_id) + ", "
                + std::to_string(axon_branch_id) + ", " + std::to_string(axon->getPosition()->x) + ", "
                + std::to_string(axon->getPosition()->y) + ", " + std::to_string(axon->getPosition()->z) + ")";
        database.exec(query);

        query = "INSERT INTO axonboutons (axon_bouton_id, axon_id, x, y, z) VALUES (" + std::to_string(axon_bouton_id)
                + ", " + std::to_string(axon_id) + ", " + std::to_string(axon->getAxonBouton()->getPosition()->x) + ", "
                + std::to_string(axon->getAxonBouton()->getPosition()->y) + ", "
                + std::to_string(axon->getAxonBouton()->getPosition()->z) + ")";
        database.exec(query);

        query = "INSERT INTO synapticgaps (synaptic_gap_id, axon_bouton_id, x, y, z) VALUES ("
                + std::to_string(synaptic_gap_id) + ", " + std::to_string(axon_bouton_id) + ", "
                + std::to_string(axon->getAxonBouton()->getSynapticGap()->getPosition()->x) + ", "
                + std::to_string(axon->getAxonBouton()->getSynapticGap()->getPosition()->y) + ", "
                + std::to_string(axon->getAxonBouton()->getSynapticGap()->getPosition()->z) + ")";
        database.exec(query);

        // Increment axon_bouton_id here after the query
        axon_bouton_id++;
        // Increment synaptic_gap_id here after the query
        synaptic_gap_id++;

        for(auto& innerAxonBranch: axon->getAxonBranches())
        {
            // Increment axon_branch_id here after the query
            axon_branch_id++;
            insertAxonBranches(database,
                               innerAxonBranch,
                               axon_branch_id,
                               axon_id,
                               axon_bouton_id,
                               synaptic_gap_id,
                               axon_hillock_id);
        }
    }
}

void runInteractor(std::vector<std::shared_ptr<Neuron>>&                     neurons,
                   std::mutex&                                               neuron_mutex,
                   ThreadSafeQueue<std::vector<std::tuple<double, double>>>& audioQueue,
                   int                                                       whichCallBack)
{
    // Initialize VTK
    // vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
    // vtkSmartPointer<vtkCellArray> lines = vtkSmartPointer<vtkCellArray>::New();
    // vtkSmartPointer<vtkPolyData> polyData = vtkSmartPointer<vtkPolyData>::New();
    // vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    // polyData->SetPoints(points);
    // polyData->SetLines(lines);
    // mapper->SetInputData(polyData);
    // vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    // vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
    // vtkSmartPointer<vtkRenderWindow> renderWindow = vtkSmartPointer<vtkRenderWindow>::New();
    // actor->SetMapper(mapper);
    // renderWindow->AddRenderer(renderer);

    if(whichCallBack == 0)
    {
        vtkSmartPointer<nvTimerCallback> nvCB = vtkSmartPointer<nvTimerCallback>::New();
        nvCB->setNeurons(neurons);
        nvCB->setMutex(neuron_mutex);
        // nvCB->setPoints(points);
        // nvCB->setLines(lines);
        // nvCB->setPolyData(polyData);
        // nvCB->setActor(actor);
        // nvCB->setMapper(mapper);
        // nvCB->setRenderer(renderer);
        nvCB->nvRenderWindow->AddRenderer(nvCB->nvRenderer);

        // Custom main loop
        while(true)
        {  // or a more appropriate loop condition
            auto executeFunc = [nvCB]() mutable { nvCB->Execute(nullptr, vtkCommand::TimerEvent, nullptr); };
            nvCB->nvRenderWindow->Render();
            // std::this_thread::sleep_for(std::chrono::milliseconds(501));
        }
    }
    if(whichCallBack == 1)
    {
        vtkSmartPointer<avTimerCallback> avCB = vtkSmartPointer<avTimerCallback>::New();
        avCB->setAudio(audioQueue);
        // avCB->setPoints(points);
        // avCB->setLines(lines);
        // avCB->setPolyData(polyData);
        // avCB->setActor(actor);
        // avCB->setMapper(mapper);
        // avCB->setRenderer(renderer);
        avCB->avRenderWindow->AddRenderer(avCB->avRenderer);

        // Custom main loop
        while(true)
        {  // or a more appropriate loop condition
            auto executeFunc = [avCB]() mutable { avCB->Execute(nullptr, vtkCommand::TimerEvent, nullptr); };
            avCB->avRenderWindow->AddRenderer(avCB->avRenderer);
            avCB->avRenderWindow->Render();
            // std::this_thread::sleep_for(std::chrono::milliseconds(99));
        }
    }
}

/**
 * @brief Main function to initialize, run, and finalize the neuron network simulation.
 * @return int Status code (0 for success, non-zero for failures).
 */
int main()
{
     std::string query;

    ThreadSafeQueue<std::vector<std::tuple<double, double>>> audioQueue;
    ThreadSafeQueue<std::vector<std::tuple<double, double>>> emptyAudioQueue;

    std::shared_ptr<PulseAudioMic> mic = std::make_shared<PulseAudioMic>(audioQueue);
    std::thread                    micThread(&PulseAudioMic::micRun, mic);

    // Read the database connection configuration and simulation configuration
    std::vector<std::string> config_filenames = {"db_connection.conf", "simulation.conf"};
    Config                   config{std::vector<std::string>{"db_connection.conf", "simulation.conf"}};

    // Get the number of neurons from the configuration
    int    num_neurons              = std::stoi(config["num_neurons"]);
    int    num_pixels               = std::stoi(config["num_pixels"]);
    int    num_phonels              = std::stoi(config["num_phonels"]);
    int    num_scentels             = std::stoi(config["num_scentels"]);
    int    num_vocels               = std::stoi(config["num_vocels"]);
    int    neuron_points_per_layer  = std::stoi(config["neuron_points_per_layer"]);
    int    pixel_points_per_layer   = std::stoi(config["pixel_points_per_layer"]);
    int    phonel_points_per_layer  = std::stoi(config["phonel_points_per_layer"]);
    int    scentel_points_per_layer = std::stoi(config["scentel_points_per_layer"]);
    int    vocel_points_per_layer   = std::stoi(config["vocel_points_per_layer"]);
    double proximityThreshold       = std::stod(config["proximity_threshold"]);
    bool   useDatabase              = convertStringToBool(config["use_database"]);

    // Connect to PostgreSQL
    DB database{config["POSTGRES_USER"],
                config["POSTGRES_PASSWORD"],
                config["POSTGRES_PORT"],
                config["POSTGRES_HOST"],
                config["POSTGRES_DB"]};
    // Set the transaction isolation level
    database.startTransaction();
    database.exec("SET TRANSACTION ISOLATION LEVEL SERIALIZABLE;");
    database.exec("SET lock_timeout = '5s';");

    // Create a single instance of NeuronParameters to access the database
    NeuronParameters params;

    // Create a list of neurons
    spdlog::info("Creating neurons...");
    std::vector<std::shared_ptr<Neuron>> neurons;
    std::mutex                           neuron_mutex;
    std::mutex                           empty_neuron_mutex;
    neurons.reserve(num_neurons);
    double                  shiftX = 0.0;
    double                  shiftY = 0.0;
    double                  shiftZ = 0.0;
    double                  newPositionX;
    double                  newPositionY;
    double                  newPositionZ;
    std::shared_ptr<Neuron> prevNeuron;
    // Currently the application places neurons in a defined pattern but the pattern is not based on cell growth
    for(int i = 0; i < num_neurons; ++i)
    {
        auto coords = get_coordinates(i, num_neurons, neuron_points_per_layer);
        if(i > 0)
        {
            prevNeuron = neurons.back();
            shiftX     = std::get<0>(coords) + (0.1 * i) + sin((3.14 / 180) * (i * 10)) * 0.1;
            shiftY     = std::get<1>(coords) + (0.1 * i) + cos((3.14 / 180) * (i * 10)) * 0.1;
            shiftZ     = std::get<2>(coords) + (0.1 * i) + sin((3.14 / 180) * (i * 10)) * 0.1;
        }

        neurons.emplace_back(std::make_shared<Neuron>(std::make_shared<Position>(shiftX, shiftY, shiftZ)));
        neurons.back()->initialise();
        // Sparsely associate neurons
        if(i > 0 && i % 3 == 0)
        {
            // First move the required gap closer to the other neuron's dendrite bouton - also need to adjust other
            // components too
            PositionPtr prevDendriteBoutonPosition =
             prevNeuron->getSoma()->getDendriteBranches()[0]->getDendrites()[0]->getDendriteBouton()->getPosition();
            PositionPtr currentSynapticGapPosition =
             neurons.back()->getSoma()->getAxonHillock()->getAxon()->getAxonBouton()->getSynapticGap()->getPosition();
            newPositionX                  = prevDendriteBoutonPosition->x + 0.4;
            newPositionY                  = prevDendriteBoutonPosition->y + 0.4;
            newPositionZ                  = prevDendriteBoutonPosition->z + 0.4;
            currentSynapticGapPosition->x = newPositionX;
            currentSynapticGapPosition->y = newPositionY;
            currentSynapticGapPosition->z = newPositionZ;
            PositionPtr currentAxonBoutonPosition =
             neurons.back()->getSoma()->getAxonHillock()->getAxon()->getAxonBouton()->getPosition();
            newPositionX                    = newPositionX + 0.4;
            newPositionY                    = newPositionY + 0.4;
            newPositionZ                    = newPositionZ + 0.4;
            currentAxonBoutonPosition->x    = newPositionX;
            currentAxonBoutonPosition->y    = newPositionY;
            currentAxonBoutonPosition->z    = newPositionZ;
            PositionPtr currentAxonPosition = neurons.back()->getSoma()->getAxonHillock()->getAxon()->getPosition();
            newPositionX                    = newPositionX + (currentAxonPosition->x - newPositionX) / 2.0;
            newPositionY                    = newPositionY + (currentAxonPosition->y - newPositionY) / 2.0;
            newPositionZ                    = newPositionZ + (currentAxonPosition->z - newPositionZ) / 2.0;
            currentAxonPosition->x          = newPositionX;
            currentAxonPosition->y          = newPositionY;
            currentAxonPosition->z          = newPositionZ;
            // Associate the synaptic gap with the other neuron's dendrite bouton if it is now close enough
            associateSynapticGap(*neurons[i - 1], *neurons[i], proximityThreshold);
        }
    }
    spdlog::info("Created {} neurons.", neurons.size());
    // Create a collection of visual inputs (pair of eyes)
    spdlog::info("Creating visual sensory inputs...");

    // Resize the vector to contain 2 elements
    std::vector<std::vector<std::shared_ptr<SensoryReceptor>>> visualInputs(2);
    spdlog::info("Created visual sensory inputs...");
    visualInputs[1].reserve(num_pixels / 2);
    std::shared_ptr<SensoryReceptor> prevReceptor;
#pragma omp parallel for
    for(int j = 0; j < 2; ++j)
    {
        for(int i = 0; i < (num_pixels / 2); ++i)
        {
            auto coords = get_coordinates(i, num_pixels, pixel_points_per_layer);
            if(i > 0)
            {
                prevReceptor = visualInputs[j].back();
                shiftX       = std::get<0>(coords) - 100 + (j * 200);
                shiftY       = std::get<1>(coords);
                shiftZ       = std::get<2>(coords) - 100;
            }

            spdlog::info("Creating visual ({}) input {} at ({}, {}, {})", j, i, shiftX, shiftY, shiftZ);
            visualInputs[j].emplace_back(
             std::make_shared<SensoryReceptor>(std::make_shared<Position>(shiftX, shiftY, shiftZ)));
            visualInputs[j].back()->initialise();
            // Sparsely associate neurons
            if(i > 0 && i % 7 == 0)
            {
                // First move the required gap closer to the other neuron's dendrite bouton - also need to adjust other
                // components too
                PositionPtr currentDendriteBoutonPosition = neurons[int(i + ((num_pixels / 2) * j))]
                                                             ->getSoma()
                                                             ->getDendriteBranches()[0]
                                                             ->getDendrites()[0]
                                                             ->getDendriteBouton()
                                                             ->getPosition();

                PositionPtr currentSynapticGapPosition = visualInputs[j].back()->getSynapticGaps()[0]->getPosition();
                newPositionX                           = currentSynapticGapPosition->x + 0.4;
                newPositionY                           = currentSynapticGapPosition->y + 0.4;
                newPositionZ                           = currentSynapticGapPosition->z + 0.4;
                currentDendriteBoutonPosition->x       = newPositionX;
                currentDendriteBoutonPosition->y       = newPositionY;
                currentDendriteBoutonPosition->z       = newPositionZ;
                PositionPtr currentDendritePosition    = neurons[int(i + ((num_pixels / 2) * j))]
                                                       ->getSoma()
                                                       ->getDendriteBranches()[0]
                                                       ->getDendrites()[0]
                                                       ->getPosition();
                newPositionX               = newPositionX + 0.4;
                newPositionY               = newPositionY + 0.4;
                newPositionZ               = newPositionZ + 0.4;
                currentDendritePosition->x = newPositionX;
                currentDendritePosition->y = newPositionY;
                currentDendritePosition->z = newPositionZ;
                // Associate the synaptic gap with the other neuron's dendrite bouton if it is now close enough
                associateSynapticGap(*visualInputs[j].back(),
                                     *neurons[int(i + ((num_pixels / 2) * j))],
                                     proximityThreshold);
            }
        }
    }
    spdlog::info("Created {} sensory receptors.", (visualInputs[0].size() + visualInputs[1].size()));

    // Create a collection of audio inputs (pair of ears)
    spdlog::info("Creating auditory sensory inputs...");

    // Resize the vector to contain 2 elements
    std::vector<std::vector<std::shared_ptr<SensoryReceptor>>> audioInputs(2);
    audioInputs[0].reserve(num_phonels);
    audioInputs[1].reserve(num_phonels);
#pragma omp parallel for
    for(int j = 0; j < 2; ++j)
    {
        for(int i = 0; i < (num_phonels / 2); ++i)
        {
            auto coords = get_coordinates(i, num_phonels, phonel_points_per_layer);
            if(i > 0)
            {
                prevReceptor = audioInputs[j].back();
                shiftX       = std::get<0>(coords) - 150 + (j * 300);
                shiftY       = std::get<1>(coords);
                shiftZ       = std::get<2>(coords);
            }

            audioInputs[j].emplace_back(
             std::make_shared<SensoryReceptor>(std::make_shared<Position>(shiftX, shiftY, shiftZ)));
            audioInputs[j].back()->initialise();
            // Sparsely associate neurons
            if(i > 0 && i % 11 == 0)
            {
                // First move the required gap closer to the other neuron's dendrite bouton - also need to adjust other
                // components too
                PositionPtr currentDendriteBoutonPosition = neurons[int(i + ((num_phonels / 2) * j))]
                                                             ->getSoma()
                                                             ->getDendriteBranches()[0]
                                                             ->getDendrites()[0]
                                                             ->getDendriteBouton()
                                                             ->getPosition();
                PositionPtr currentSynapticGapPosition = audioInputs[j].back()->getSynapticGaps()[0]->getPosition();
                newPositionX                           = currentSynapticGapPosition->x + 0.4;
                newPositionY                           = currentSynapticGapPosition->y + 0.4;
                newPositionZ                           = currentSynapticGapPosition->z + 0.4;
                currentDendriteBoutonPosition->x       = newPositionX;
                currentDendriteBoutonPosition->y       = newPositionY;
                currentDendriteBoutonPosition->z       = newPositionZ;
                PositionPtr currentDendritePosition    = neurons[int(i + ((num_phonels / 2) * j))]
                                                       ->getSoma()
                                                       ->getDendriteBranches()[0]
                                                       ->getDendrites()[0]
                                                       ->getPosition();
                newPositionX               = newPositionX + 0.4;
                newPositionY               = newPositionY + 0.4;
                newPositionZ               = newPositionZ + 0.4;
                currentDendritePosition->x = newPositionX;
                currentDendritePosition->y = newPositionY;
                currentDendritePosition->z = newPositionZ;
                // Associate the synaptic gap with the other neuron's dendrite bouton if it is now close enough
                associateSynapticGap(*audioInputs[j].back(),
                                     *neurons[int(i + ((num_phonels / 2) * j))],
                                     proximityThreshold);
            }
        }
    }
    spdlog::info("Created {} sensory receptors.", (audioInputs[0].size() + audioInputs[1].size()));

    // Create a collection of olfactory inputs (pair of nostrils)
    spdlog::info("Creating olfactory sensory inputs...");

    // Resize the vector to contain 2 elements
    std::vector<std::vector<std::shared_ptr<SensoryReceptor>>> olfactoryInputs(2);
    olfactoryInputs[0].reserve(num_scentels);
    olfactoryInputs[1].reserve(num_scentels);
#pragma omp parallel for
    for(int j = 0; j < 2; ++j)
    {
        for(int i = 0; i < (num_scentels / 2); ++i)
        {
            auto coords = get_coordinates(i, num_scentels, scentel_points_per_layer);
            if(i > 0)
            {
                prevReceptor = olfactoryInputs[j].back();
                shiftX       = std::get<0>(coords) - 20 + (j * 40);
                shiftY       = std::get<1>(coords) - 10;
                shiftZ       = std::get<2>(coords) - 10;
            }

            olfactoryInputs[j].emplace_back(
             std::make_shared<SensoryReceptor>(std::make_shared<Position>(shiftX, shiftY, shiftZ)));
            olfactoryInputs[j].back()->initialise();
            // Sparsely associate neurons
            if(i > 0 && i % 13 == 0)
            {
                // First move the required gap closer to the other neuron's dendrite bouton - also need to adjust other
                // components too
                PositionPtr currentDendriteBoutonPosition = neurons[int(i + ((num_scentels / 2) * j))]
                                                             ->getSoma()
                                                             ->getDendriteBranches()[0]
                                                             ->getDendrites()[0]
                                                             ->getDendriteBouton()
                                                             ->getPosition();
                PositionPtr currentSynapticGapPosition = olfactoryInputs[j].back()->getSynapticGaps()[0]->getPosition();
                newPositionX                           = currentSynapticGapPosition->x + 0.4;
                newPositionY                           = currentSynapticGapPosition->y + 0.4;
                newPositionZ                           = currentSynapticGapPosition->z + 0.4;
                currentDendriteBoutonPosition->x       = newPositionX;
                currentDendriteBoutonPosition->y       = newPositionY;
                currentDendriteBoutonPosition->z       = newPositionZ;
                PositionPtr currentDendritePosition    = neurons[int(i + ((num_scentels / 2) * j))]
                                                       ->getSoma()
                                                       ->getDendriteBranches()[0]
                                                       ->getDendrites()[0]
                                                       ->getPosition();
                newPositionX               = newPositionX + 0.4;
                newPositionY               = newPositionY + 0.4;
                newPositionZ               = newPositionZ + 0.4;
                currentDendritePosition->x = newPositionX;
                currentDendritePosition->y = newPositionY;
                currentDendritePosition->z = newPositionZ;
                // Associate the synaptic gap with the other neuron's dendrite bouton if it is now close enough
                associateSynapticGap(*olfactoryInputs[j].back(),
                                     *neurons[int(i + ((num_scentels / 2) * j))],
                                     proximityThreshold);
            }
        }
    }
    spdlog::info("Created {} sensory receptors.", (olfactoryInputs[0].size() + olfactoryInputs[1].size()));

    // Create a collection of vocal effector outputs (vocal tract cords/muscle control)
    spdlog::info("Creating vocal effector outputs...");
    std::vector<std::shared_ptr<Effector>> vocalOutputs;
    vocalOutputs.reserve(num_vocels);
    std::shared_ptr<Effector> prevEffector;

    for(int i = 0; i < (num_vocels); ++i)
    {
        auto coords = get_coordinates(i, num_vocels, vocel_points_per_layer);
        if(i > 0)
        {
            prevEffector = vocalOutputs.back();
            shiftX       = std::get<0>(coords);
            shiftY       = std::get<1>(coords) - 100;
            shiftZ       = std::get<2>(coords) + 10;
        }

        vocalOutputs.emplace_back(std::make_shared<Effector>(std::make_shared<Position>(shiftX, shiftY, shiftZ)));
        vocalOutputs.back()->initialise();
        // Sparsely associate neurons
        if(i > 0 && i % 17 == 0
           && !neurons[int(i + num_vocels)]
                ->getSoma()
                ->getAxonHillock()
                ->getAxon()
                ->getAxonBouton()
                ->getSynapticGap()
                ->isAssociated())
        {
            // First move the required gap closer to the other neuron's dendrite bouton - also need to adjust other
            // components too
            PositionPtr currentSynapticGapPosition = neurons[int(i + num_vocels)]
                                                      ->getSoma()
                                                      ->getAxonHillock()
                                                      ->getAxon()
                                                      ->getAxonBouton()
                                                      ->getSynapticGap()
                                                      ->getPosition();
            PositionPtr currentEffectorPosition = vocalOutputs.back()->getPosition();
            newPositionX                        = currentEffectorPosition->x - 0.4;
            newPositionY                        = currentEffectorPosition->y - 0.4;
            newPositionZ                        = currentEffectorPosition->z - 0.4;
            currentSynapticGapPosition->x       = newPositionX;
            currentSynapticGapPosition->y       = newPositionY;
            currentSynapticGapPosition->z       = newPositionZ;
            PositionPtr currentAxonBoutonPosition =
             neurons[int(i + num_vocels)]->getSoma()->getAxonHillock()->getAxon()->getAxonBouton()->getPosition();
            newPositionX                 = newPositionX + 0.4;
            newPositionY                 = newPositionY + 0.4;
            newPositionZ                 = newPositionZ + 0.4;
            currentAxonBoutonPosition->x = newPositionX;
            currentAxonBoutonPosition->y = newPositionY;
            currentAxonBoutonPosition->z = newPositionZ;
            PositionPtr currentAxonPosition =
             neurons[int(i + num_vocels)]->getSoma()->getAxonHillock()->getAxon()->getPosition();
            newPositionX           = newPositionX + 0.4;
            newPositionY           = newPositionY + 0.4;
            newPositionZ           = newPositionZ + 0.4;
            currentAxonPosition->x = newPositionX;
            currentAxonPosition->y = newPositionY;
            currentAxonPosition->z = newPositionZ;
            // Associate the synaptic gap with the other neuron's dendrite bouton if it is now close enough
            neurons[int(i + num_vocels)]
             ->getSoma()
             ->getAxonHillock()
             ->getAxon()
             ->getAxonBouton()
             ->getSynapticGap()
             ->setAsAssociated();
        }
    }
    spdlog::info("Created {} effectors.", vocalOutputs.size());

// Assuming neurons is a std::vector<Neuron*> or similar container
// and associateSynapticGap is a function you've defined elsewhere

// Parallelize synaptic association
#pragma omp parallel for schedule(dynamic)
    for(size_t i = 0; i < neurons.size(); ++i)
    {
        for(size_t j = i + 1; j < neurons.size(); ++j)
        {
            associateSynapticGap(*neurons[i], *neurons[j], proximityThreshold);
        }
    }

    // Use a fixed number of threads
    const size_t numThreads = std::thread::hardware_concurrency();

    std::vector<std::thread> threads;
    threads.reserve(numThreads);

    size_t neuronsPerThread = neurons.size() / numThreads;

    // Calculate the propagation rate in parallel
    for(size_t t = 0; t < numThreads; ++t)
    {
        size_t start = t * neuronsPerThread;
        size_t end   = (t + 1) * neuronsPerThread;
        if(t == numThreads - 1)
            end = neurons.size();  // Handle remainder

        threads.emplace_back(
         [=, &neurons]()
         {
             for(size_t i = start; i < end; ++i)
             {
                 computePropagationRate(neurons[i]);
             }
         });
    }

    // Join the threads
    for(std::thread& t: threads)
    {
        t.join();
    }

    // Get the total propagation rate
    double propagationRate = totalPropagationRate.load();

    spdlog::info("The propagation rate is {}", propagationRate);

    int neuron_id          = 0;
    int soma_id            = 0;
    int axon_hillock_id    = 0;
    int axon_id            = 0;
    int axon_bouton_id     = 0;
    int axon_branch_id     = 0;
    int synaptic_gap_id    = 0;
    int dendrite_id        = 0;
    int dendrite_bouton_id = 0;
    int dendrite_branch_id = 0;

    for(auto& neuron: neurons)
    {
        query = "INSERT INTO neurons (neuron_id, x, y, z) VALUES (" + std::to_string(neuron_id) + ", "
                + std::to_string(neuron->getPosition()->x) + ", " + std::to_string(neuron->getPosition()->y) + ", "
                + std::to_string(neuron->getPosition()->z) + ")";
        database.exec(query);
        query = "INSERT INTO somas (soma_id, neuron_id, x, y, z) VALUES (" + std::to_string(soma_id) + ", "
                + std::to_string(neuron_id) + ", " + std::to_string(neuron->getSoma()->getPosition()->x) + ", "
                + std::to_string(neuron->getSoma()->getPosition()->y) + ", "
                + std::to_string(neuron->getSoma()->getPosition()->z) + ")";
        database.exec(query);
        query = "INSERT INTO axonhillocks (axon_hillock_id, soma_id, x, y, z) VALUES ("
                + std::to_string(axon_hillock_id) + ", " + std::to_string(soma_id) + ", "
                + std::to_string(neuron->getSoma()->getAxonHillock()->getPosition()->x) + ", "
                + std::to_string(neuron->getSoma()->getAxonHillock()->getPosition()->y) + ", "
                + std::to_string(neuron->getSoma()->getAxonHillock()->getPosition()->z) + ")";
        database.exec(query);
        query = "INSERT INTO axons (axon_id, axon_hillock_id, x, y, z) VALUES (" + std::to_string(axon_id) + ", "
                + std::to_string(axon_hillock_id) + ", "
                + std::to_string(neuron->getSoma()->getAxonHillock()->getAxon()->getPosition()->x) + ", "
                + std::to_string(neuron->getSoma()->getAxonHillock()->getAxon()->getPosition()->y) + ", "
                + std::to_string(neuron->getSoma()->getAxonHillock()->getAxon()->getPosition()->z) + ")";
        database.exec(query);
        query =
         "INSERT INTO axonboutons (axon_bouton_id, axon_id, x, y, z) VALUES (" + std::to_string(axon_bouton_id) + ", "
         + std::to_string(axon_id) + ", "
         + std::to_string(neuron->getSoma()->getAxonHillock()->getAxon()->getAxonBouton()->getPosition()->x) + ", "
         + std::to_string(neuron->getSoma()->getAxonHillock()->getAxon()->getAxonBouton()->getPosition()->y) + ", "
         + std::to_string(neuron->getSoma()->getAxonHillock()->getAxon()->getAxonBouton()->getPosition()->z) + ")";
        database.exec(query);
        query = "INSERT INTO synapticgaps (synaptic_gap_id, axon_bouton_id, x, y, z) VALUES ("
                + std::to_string(synaptic_gap_id) + ", " + std::to_string(axon_bouton_id) + ", "
                + std::to_string(
                 neuron->getSoma()->getAxonHillock()->getAxon()->getAxonBouton()->getSynapticGap()->getPosition()->x)
                + ", "
                + std::to_string(
                 neuron->getSoma()->getAxonHillock()->getAxon()->getAxonBouton()->getSynapticGap()->getPosition()->y)
                + ", "
                + std::to_string(
                 neuron->getSoma()->getAxonHillock()->getAxon()->getAxonBouton()->getSynapticGap()->getPosition()->z)
                + ")";
        database.exec(query);
        axon_bouton_id++;
        synaptic_gap_id++;

        if(neuron && neuron->getSoma() && neuron->getSoma()->getAxonHillock()
           && neuron->getSoma()->getAxonHillock()->getAxon()
           && !neuron->getSoma()->getAxonHillock()->getAxon()->getAxonBranches().empty()
           && neuron->getSoma()->getAxonHillock()->getAxon()->getAxonBranches()[0])
        {
            for(auto& axonBranch: neuron->getSoma()->getAxonHillock()->getAxon()->getAxonBranches())
            {
                insertAxonBranches(database,
                                   axonBranch,
                                   axon_branch_id,
                                   axon_id,
                                   axon_bouton_id,
                                   synaptic_gap_id,
                                   axon_hillock_id);
                axon_branch_id++;
            }
        }

        for(auto& dendriteBranch: neuron->getSoma()->getDendriteBranches())
        {
            insertDendriteBranches(database,
                                   dendriteBranch,
                                   dendrite_branch_id,
                                   dendrite_id,
                                   dendrite_bouton_id,
                                   soma_id);
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
    if(propagationRate != 0)
    {
        // std::string query = "INSERT INTO neurons (propagation_rate, neuron_type, axon_length) VALUES (" +
        // std::to_string(propagationRate) + ", 0, 0)"; txn.exec(query);
        database.commitTransaction();
    }
    else
    {
        throw std::runtime_error("The propagation rate is not valid. Skipping database insertion.");
    }

    std::vector<std::shared_ptr<Neuron>> emptyNeurons;
    // Set up and start the interactors in separate threads
    std::thread nvThread(runInteractor, std::ref(neurons), std::ref(neuron_mutex), std::ref(emptyAudioQueue), 0);
    std::thread avThread(runInteractor, std::ref(emptyNeurons), std::ref(empty_neuron_mutex), std::ref(audioQueue), 1);
    std::thread inputThread(checkForQuit);

    while(running)
    {
        std::cout << "v";
        std::this_thread::sleep_for(std::chrono::milliseconds(100));  // sleep for 100 ms
    }

    // Wait for both threads to finish
    inputThread.join();
    nvThread.join();
    avThread.join();
    mic->micStop();
    micThread.join();

    return 0;
}
