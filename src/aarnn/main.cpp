//
// Created by pbisaacs on 23/06/24.
//
#include <iostream>
#include <vector>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <pqxx/pqxx>
#include "Logger.h"
#include "config.h"
#include "database.h"
#include <poll.h>
#include <unistd.h>
#include "utils.h"
#include "vclient.h"
#include "NeuronParameters.h"
#include "globals.h"

std::atomic<double> totalPropagationRate(0.0);
std::mutex mtx;
std::unordered_set<std::shared_ptr<Neuron>> changedNeurons;
std::mutex changedNeuronsMutex;
std::atomic<bool> running(true);
std::condition_variable cv;
std::atomic<bool> dbUpdateReady(false);

// Configuration and Logger Initialization

// Function to simulate logging from multiple threads
void logMessages(Logger& logger, int thread_id) {
    for (int i = 0; i < 5; ++i) {
        logger << "Thread " << thread_id << " logging message " << i << std::endl;
    }
}

void checkForQuit() {
    struct pollfd fds[1];
    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;

    while (running) {
        int ret = poll(fds, 1, 1000); // 1000ms timeout

        if (ret > 0) {
            char key;
            read(STDIN_FILENO, &key, 1);
            if (key == 'q') {
                running = false;
                dbUpdateReady = true;
                cv.notify_all();
                break;
            }
        } else if (ret < 0) {
            // Handle error
        }

        // Else timeout occurred, just loop back and poll again
    }
}

void computePropagationRate(const std::shared_ptr<Neuron>& neuron) {
    if (!neuron) return; // Add check for null neuron
    auto soma = neuron->getSoma();
    if (!soma) return; // Add check for null soma
    double propagationRate = soma->getPropagationRate();

    atomic_add(totalPropagationRate, propagationRate);

    {
        std::lock_guard<std::mutex> lock(changedNeuronsMutex);
        changedNeurons.insert(neuron);
    }
}


int main() {
    // Initialize Logger
    Logger logger("errors_aarnn.log");

    std::thread t1(logMessages, std::ref(logger), 1);


    std::string input = "Hello, World!";
    std::string encoded = base64_encode(reinterpret_cast<const unsigned char*>(input.c_str()), input.length());

    std::cout << "Base64 Encoded: " << encoded << std::endl;
    std::string query;

    std::vector<std::string> config_filenames = { "simulation.conf" };
    auto config = read_config(config_filenames);

    std::string connection_string;

    if (!initialiseDatabaseConnection(connection_string)) {
        std::cerr << "Failed to initialise database connection." << std::endl;
        return 1;
    }
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

    pqxx::connection conn(connection_string);
    initialise_database(conn);
    std::cout << "1" << std::endl;
    pqxx::work txn(conn);
    std::cout << "2" << std::endl;
    txn.exec("SET TRANSACTION ISOLATION LEVEL SERIALIZABLE;");
    std::cout << "3" << std::endl;
    txn.exec("SET lock_timeout = '5s';");
    std::cout << "4" << std::endl;

    NeuronParameters params;
    std::cout << "5" << std::endl;

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
    std::cout << "6" << std::endl;
    for (int i = 0; i < num_neurons; ++i) {
        std::cout << "7" << std::endl;
        auto coords = get_coordinates(i, num_neurons, neuron_points_per_layer);
        std::cout << "8" << std::endl;
        if (i > 0) {
            prevNeuron = neurons.back();
            shiftX = std::get<0>(coords) + (0.1 * i) + sin((3.14 / 180) * (i * 10)) * 0.1;
            shiftY = std::get<1>(coords) + (0.1 * i) + cos((3.14 / 180) * (i * 10)) * 0.1;
            shiftZ = std::get<2>(coords) + (0.1 * i) + sin((3.14 / 180) * (i * 10)) * 0.1;
        }

        neurons.emplace_back(std::make_shared<Neuron>(std::make_shared<Position>(shiftX, shiftY, shiftZ)));
        std::cout << "9" << std::endl;
        neurons.back()->initialise();
        std::cout << "10" << std::endl;
        neurons.back()->setPropagationRate(1.0);
        if (i > 0 && i % 3 == 0) {
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
            associateSynapticGap(*neurons[i - 1], *neurons[i], proximityThreshold);
        }
    }
    std::cout << "Created " << neurons.size() << " neurons." << std::endl;

    std::vector<std::vector<std::shared_ptr<SensoryReceptor>>> visualInputs(2);
    visualInputs[0].reserve(num_pixels / 2);
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

            std::cout << "Creating visual (" << j << ") input " << i << " at (" << shiftX << ", " << shiftY << ", " << shiftZ << ")" << std::endl;
            visualInputs[j].emplace_back(std::make_shared<SensoryReceptor>(std::make_shared<Position>(shiftX, shiftY, shiftZ)));
            if (!visualInputs[j].back()) {
                std::cerr << "visualInputs[" << j << "].back() is null!" << std::endl;
                continue;
            }
            visualInputs[j].back()->initialise();
            if (i > 0 && i % 7 == 0) {
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
                associateSynapticGap(*visualInputs[j].back(), *neurons[int(i + ((num_pixels / 2) * j))], proximityThreshold);
            }
        }
    }
    std::cout << "Created " << ( visualInputs[0].size() + visualInputs[1].size()) << " sensory receptors." << std::endl;

    std::vector<std::vector<std::shared_ptr<SensoryReceptor>>> audioInputs(2);
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

            audioInputs[j].emplace_back(std::make_shared<SensoryReceptor>(std::make_shared<Position>(shiftX, shiftY, shiftZ)));
            audioInputs[j].back()->initialise();
            if (i > 0 && i % 11 == 0) {
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
                associateSynapticGap(*audioInputs[j].back(), *neurons[int(i + ((num_phonels / 2) * j))], proximityThreshold);
            }
        }
    }
    std::cout << "Created " << ( audioInputs[0].size() + audioInputs[1].size()) << " sensory receptors." << std::endl;

    std::vector<std::vector<std::shared_ptr<SensoryReceptor>>> olfactoryInputs(2);
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

            olfactoryInputs[j].emplace_back(std::make_shared<SensoryReceptor>(std::make_shared<Position>(shiftX, shiftY, shiftZ)));
            olfactoryInputs[j].back()->initialise();
            if (i > 0 && i % 13 == 0) {
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
                associateSynapticGap(*olfactoryInputs[j].back(), *neurons[int(i + ((num_scentels / 2) * j))], proximityThreshold);
            }
        }
    }
    std::cout << "Created " << ( olfactoryInputs[0].size() + olfactoryInputs[1].size()) << " sensory receptors." << std::endl;

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

        vocalOutputs.emplace_back(std::make_shared<Effector>(std::make_shared<Position>(shiftX, shiftY, shiftZ)));
        vocalOutputs.back()->initialise();
        if (i > 0 && i % 17 == 0 && !neurons[int(i + num_vocels)]->getSoma()->getAxonHillock()->getAxon()->getAxonBouton()->getSynapticGap()->isAssociated()) {
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
            neurons[int(i + num_vocels)]->getSoma()->getAxonHillock()->getAxon()->getAxonBouton()->getSynapticGap()->setAsAssociated();
        }
    }
    std::cout << "Created " << vocalOutputs.size() << " effectors." << std::endl;

#pragma omp parallel for schedule(dynamic)
    for (size_t i = 0; i < neurons.size(); ++i) {
        for (size_t j = i + 1; j < neurons.size(); ++j) {
            associateSynapticGap(*neurons[i], *neurons[j], proximityThreshold);
        }
    }

    const size_t numThreads = std::thread::hardware_concurrency();
    std::vector<std::thread> threads(numThreads);

    size_t neuronsPerThread = neurons.size() / numThreads;

    for (size_t t = 0; t < numThreads; ++t) {
        size_t start = t * neuronsPerThread;
        size_t end = (t + 1) * neuronsPerThread;
        if (t == numThreads - 1) end = neurons.size();

        threads[t] = std::thread([start, end, &neurons]() {
            for (size_t i = start; i < end; ++i) {
                computePropagationRate(neurons[i]);
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    double propagationRate = totalPropagationRate.load();
    std::cout << "The propagation rate is " << propagationRate << std::endl;

    try {
        batch_insert_neurons(txn, neurons);
        std::cout << "Batch insertion completed." << std::endl;
        std::cout << "Total Propagation Rate: " << propagationRate << std::endl;
        if (propagationRate != 0) {
            txn.commit();
        } else {
            std::cout << "Propagation rate is zero. Aborting transaction." << std::endl;
            throw std::runtime_error("The propagation rate is not valid. Skipping database insertion.");
        }
    } catch (const std::exception& e) {
        std::cerr << "Database error: " << e.what() << std::endl;
        txn.abort();
        std::cout << "Transaction aborted." << std::endl;
    }

    std::vector<std::shared_ptr<Neuron>> emptyNeurons;
    //std::thread nvThread(runInteractor, std::ref(neurons), std::ref(neuron_mutex), std::ref(emptyAudioQueue), 0);
    //std::thread avThread(runInteractor, std::ref(emptyNeurons), std::ref(empty_neuron_mutex), std::ref(audioQueue), 1);
    std::thread inputThread(checkForQuit);
    std::thread dbThread(updateDatabase, std::ref(conn));

    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    inputThread.join();
    //nvThread.join();
    //avThread.join();
    //mic->micStop();
    //micThread.join();
    dbThread.join();

    return 0;
}
