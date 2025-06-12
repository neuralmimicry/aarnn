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
#include <cmath>
#include "Cluster.h"
#include "SensoryReceptor.h"
#include "Effector.h"
#include "Neuron.h"
#include "AuditoryManager.h"
#include "SensoryReceptorServer.h"


//std::atomic<double> totalPropagationRate(0.0);
std::mutex mtx;
std::unordered_set<std::shared_ptr<Neuron>> changedNeurons;
std::unordered_set<std::shared_ptr<Cluster>> changedClusters;
std::mutex changedNeuronsMutex;
std::atomic<bool> running(true);
std::condition_variable cv;
std::atomic<bool> dbUpdateReady(false);
std::mutex clustersMutex;

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
            ssize_t n = read(STDIN_FILENO, &key, 1);
            if (n < 0) {
                perror("read");
                break;            // or handle the error
            } else if (n == 0) {
                // EOF, maybe also break
                break;
            }
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

double computePropagationRate(const std::shared_ptr<Neuron>& neuron) {
    if (!neuron) return 0.0; // Return 0.0 for null neuron
    auto soma = neuron->getSoma();
    if (!soma) return 0.0; // Return 0.0 for null soma

    // Compute the propagation rate (replace with actual computation)
    double propagationRate = 1.0; // Placeholder value
    neuron->setPropagationRate(propagationRate);

    //{
    //    std::lock_guard<std::mutex> lock(changedNeuronsMutex);
    //    changedNeurons.insert(neuron);
    //}

    return propagationRate;
}

void updateClusters(std::vector<std::shared_ptr<Cluster>>& clusters, std::atomic<bool>& clusterRunning) {
    auto lastTime = std::chrono::steady_clock::now();
    while (clusterRunning) {
        auto currentTime = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsedTime = currentTime - lastTime;
        double deltaTime = elapsedTime.count();
        lastTime = currentTime;

        // Update each cluster with the new deltaTime
        for (auto& cluster : clusters) {
            if (cluster) {
                cluster->update(deltaTime);
            }
        }

        // Signal database update
        {
            std::lock_guard<std::mutex> lock(changedNeuronsMutex);
            dbUpdateReady = true;
        }
        cv.notify_one();

        // Sleep for 250 milliseconds
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }

    // Signal database thread to exit
    {
        std::lock_guard<std::mutex> lock(changedNeuronsMutex);
        dbUpdateReady = true;
    }
    cv.notify_one();
}

int main() {
    // Initialise Logger
    Logger logger("errors_aarnn.log");
    std::thread t1(logMessages, std::ref(logger), 1);

    std::string input = "AARNN Starting up...";
    std::string encoded = base64_encode(reinterpret_cast<const unsigned char *>(input.c_str()), input.length());
    std::cout << "Base64 Encoded: " << encoded << std::endl;

    std::vector<std::string> config_filenames = {"simulation.conf"};
    auto config = read_config(config_filenames);

    std::string connection_string;
    bool dbAvailable = false;

    // Attempt to initialise database connection
    if (!initialiseDatabaseConnection(connection_string)) {
        std::cerr << "[WARNING] Failed to initialise database connection. Continuing without database." << std::endl;
    } else {
        try {
            // Wrap pqxx::connection construction in try/catch
            // so that if the connection fails, we disable database usage but do not crash
            pqxx::connection conn_test(connection_string);
            conn_test.close();
            dbAvailable = true;
        } catch (const std::exception& e) {
            std::cerr << "[WARNING] Cannot connect to PostgreSQL (" << e.what() << "). Continuing without database." << std::endl;
        }
    }

    const int FFT_SIZE = 1024;
    const int NUM_RECEPTORS = FFT_SIZE / 2 + 1;

    // Initialise SensoryReceptorServer
    SensoryReceptorServer receptorServer;
    if (!receptorServer.initialise()) {
        std::cerr << "Failed to initialise Sensory Receptor Server." << std::endl;
    }

    int num_clusters = std::stoi(config["num_clusters"]);
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
    double deltaTime = 0.01; // Time step in seconds (10 milliseconds)

    // If the user requested database but it's unavailable, log a warning
    if (convertStringToBool(config["use_database"]) && !dbAvailable) {
        std::cerr << "[WARNING] Database operations are disabled; 'use_database' is true but connection failed." << std::endl;
    }

    // Only set up database if available
    std::unique_ptr<pqxx::connection> conn_ptr;
    std::unique_ptr<pqxx::connection> conn_ptr_updates;
    if (useDatabase) {
        try {
            // Separate connections for updates and initial setup
            conn_ptr = std::make_unique<pqxx::connection>(connection_string);
            initialise_database(*conn_ptr);
            prepareAllStatements(*conn_ptr);
            conn_ptr_updates = std::make_unique<pqxx::connection>(connection_string);
            prepareAllStatements(*conn_ptr_updates);
            // Begin first transaction
            // We create an explicit work txn pointer so that later code can check if it's valid
        } catch (const std::exception& e) {
            std::cerr << "[WARNING] Exception during database setup: " << e.what() << ". Disabling database." << std::endl;
            useDatabase = false;
            conn_ptr.reset();
            conn_ptr_updates.reset();
        }
    }

    // Create clusters
    std::vector<std::shared_ptr<Cluster>> clusters;
    clusters.reserve(num_clusters);

    for (int i = 0; i < num_clusters; ++i) {
        // Create a cluster with a position that is at least 100 units away from previous clusters
        auto cluster = Cluster::createCluster(100.0);
        cluster->initialise(num_neurons, neuron_points_per_layer, proximityThreshold);
        cluster->setPropagationRate(1.0);
        clusters.emplace_back(cluster);
    }

    std::cout << "Created " << clusters.size() << " clusters." << std::endl;

    // Flatten all neurons from all clusters into a single vector for easy access
    std::vector<std::shared_ptr<Neuron>> allNeurons;
    for (const auto &cluster: clusters) {
        const auto &neuronsInCluster = cluster->getNeurons();
        allNeurons.insert(allNeurons.end(), neuronsInCluster.begin(), neuronsInCluster.end());
    }

    // std::cout << "Debug Step 1." << std::endl;
    // Create visual inputs
    std::vector<std::vector<std::shared_ptr<SensoryReceptor>>> visualReceptors(2);
    visualReceptors[0].reserve(num_pixels / 2);
    visualReceptors[1].reserve(num_pixels / 2);

    double shiftX, shiftY, shiftZ;
    double newPositionX, newPositionY, newPositionZ;
    // std::cout << "Debug Step 2." << std::endl;

    for (int j = 0; j < 2; ++j) {
        for (int i = 0; i < (num_pixels / 2); ++i) {
            auto coords = get_coordinates(i, num_pixels, pixel_points_per_layer);

            shiftX = std::get<0>(coords) - 100 + (j * 200);
            shiftY = std::get<1>(coords);
            shiftZ = std::get<2>(coords) - 100;

            auto receptorPosition = std::make_shared<Position>(shiftX, shiftY, shiftZ);
            auto receptor = std::make_shared<SensoryReceptor>(receptorPosition);
            receptor->initialise();
            visualReceptors[j].emplace_back(receptor);

            // Connect the receptor to neurons across all clusters
            if (i > 0 && i % 7 == 0) {
                // Calculate the neuron index to connect to
                int neuronIndex = (i + ((num_pixels / 2) * j)) % allNeurons.size();
                auto neuron = allNeurons[neuronIndex];
                // std::cout << "Debug Step 3." << std::endl;

                // Adjust positions for connection
                PositionPtr currentDendriteBoutonPosition = neuron->getSoma()->getDendriteBranches()[0]
                        ->getDendrites()[0]->getDendriteBouton()->getPosition();
                // std::cout << "Debug Step 3a." << std::endl;
                PositionPtr currentSynapticGapPosition = receptor->getSynapticGaps()[0]->getPosition();

                // std::cout << "Debug Step 4." << std::endl;
                newPositionX = currentSynapticGapPosition->x + 0.4;
                newPositionY = currentSynapticGapPosition->y + 0.4;
                newPositionZ = currentSynapticGapPosition->z + 0.4;

                currentDendriteBoutonPosition->x = newPositionX;
                currentDendriteBoutonPosition->y = newPositionY;
                currentDendriteBoutonPosition->z = newPositionZ;

                PositionPtr currentDendritePosition = neuron->getSoma()->getDendriteBranches()[0]
                        ->getDendrites()[0]->getPosition();

                newPositionX += 0.4;
                newPositionY += 0.4;
                newPositionZ += 0.4;

                currentDendritePosition->x = newPositionX;
                currentDendritePosition->y = newPositionY;
                currentDendritePosition->z = newPositionZ;
                // std::cout << "Debug Step 5." << std::endl;

                // Associate synaptic gap
                associateSynapticGap(*receptor, *neuron, proximityThreshold);
            }
        }
    }

    std::cout << "Created " << (visualReceptors[0].size() + visualReceptors[1].size()) << " visual sensory receptors."
              << std::endl;

    // Create audio inputs (left/right “hemispheres”)
    std::vector<std::vector<std::shared_ptr<SensoryReceptor>>> auditoryReceptors(2);
    auditoryReceptors[0].reserve(num_phonels / 2);
    auditoryReceptors[1].reserve(num_phonels / 2);

    for (int j = 0; j < 2; ++j) {
        for (int i = 0; i < (num_phonels / 2); ++i) {
            auto coords = get_coordinates(i, num_phonels, phonel_points_per_layer);

            shiftX = std::get<0>(coords) - 150 + (j * 300);
            shiftY = std::get<1>(coords);
            shiftZ = std::get<2>(coords);

            auto receptorPosition = std::make_shared<Position>(shiftX, shiftY, shiftZ);
            auto receptor = std::make_shared<SensoryReceptor>(receptorPosition);
            receptor->initialise();
            auditoryReceptors[j].push_back(receptor);

            // Connect the receptor to neurons across all clusters
            if (i > 0 && i % 11 == 0) {
                // Calculate the neuron index to connect to
                int neuronIndex = (i + ((num_phonels / 2) * j)) % allNeurons.size();
                auto neuron = allNeurons[neuronIndex];

                // Adjust positions for connection
                PositionPtr currentDendriteBoutonPosition = neuron->getSoma()->getDendriteBranches()[0]
                        ->getDendrites()[0]->getDendriteBouton()->getPosition();
                PositionPtr currentSynapticGapPosition = receptor->getSynapticGaps()[0]->getPosition();

                newPositionX = currentSynapticGapPosition->x + 0.4;
                newPositionY = currentSynapticGapPosition->y + 0.4;
                newPositionZ = currentSynapticGapPosition->z + 0.4;

                currentDendriteBoutonPosition->x = newPositionX;
                currentDendriteBoutonPosition->y = newPositionY;
                currentDendriteBoutonPosition->z = newPositionZ;

                PositionPtr currentDendritePosition = neuron->getSoma()->getDendriteBranches()[0]
                        ->getDendrites()[0]->getPosition();

                newPositionX += 0.4;
                newPositionY += 0.4;
                newPositionZ += 0.4;

                currentDendritePosition->x = newPositionX;
                currentDendritePosition->y = newPositionY;
                currentDendritePosition->z = newPositionZ;

                // Associate synaptic gap
                associateSynapticGap(*receptor, *neuron, proximityThreshold);
            }
        }
    }

    std::cout << "Created "
              << (auditoryReceptors[0].size() + auditoryReceptors[1].size())
              << " audio sensory receptors." << std::endl;

    // Create olfactory inputs
    std::vector<std::vector<std::shared_ptr<SensoryReceptor>>> olfactoryReceptors(2);
    olfactoryReceptors[0].reserve(num_scentels / 2);
    olfactoryReceptors[1].reserve(num_scentels / 2);

    for (int j = 0; j < 2; ++j) {
        for (int i = 0; i < (num_scentels / 2); ++i) {
            auto coords = get_coordinates(i, num_scentels, scentel_points_per_layer);

            shiftX = std::get<0>(coords) - 20 + (j * 40);
            shiftY = std::get<1>(coords) - 10;
            shiftZ = std::get<2>(coords) - 10;

            auto receptorPosition = std::make_shared<Position>(shiftX, shiftY, shiftZ);
            auto receptor = std::make_shared<SensoryReceptor>(receptorPosition);
            receptor->initialise();
            olfactoryReceptors[j].push_back(receptor);

            // Connect the receptor to neurons across all clusters
            if (i > 0 && i % 13 == 0) {
                // Calculate the neuron index to connect to
                int neuronIndex = (i + ((num_scentels / 2) * j)) % allNeurons.size();
                auto neuron = allNeurons[neuronIndex];

                // Adjust positions for connection
                PositionPtr currentDendriteBoutonPosition = neuron->getSoma()->getDendriteBranches()[0]
                        ->getDendrites()[0]->getDendriteBouton()->getPosition();
                PositionPtr currentSynapticGapPosition = receptor->getSynapticGaps()[0]->getPosition();

                newPositionX = currentSynapticGapPosition->x + 0.4;
                newPositionY = currentSynapticGapPosition->y + 0.4;
                newPositionZ = currentSynapticGapPosition->z + 0.4;

                currentDendriteBoutonPosition->x = newPositionX;
                currentDendriteBoutonPosition->y = newPositionY;
                currentDendriteBoutonPosition->z = newPositionZ;

                PositionPtr currentDendritePosition = neuron->getSoma()->getDendriteBranches()[0]
                        ->getDendrites()[0]->getPosition();

                newPositionX += 0.4;
                newPositionY += 0.4;
                newPositionZ += 0.4;

                currentDendritePosition->x = newPositionX;
                currentDendritePosition->y = newPositionY;
                currentDendritePosition->z = newPositionZ;

                // Associate synaptic gap
                associateSynapticGap(*receptor, *neuron, proximityThreshold);
            }
        }
    }

    std::cout << "Created " << (olfactoryReceptors[0].size() + olfactoryReceptors[1].size())
              << " olfactory sensory receptors." << std::endl;

    // Create effectors
    std::vector<std::shared_ptr<Effector>> vocalOutputs;
    vocalOutputs.reserve(num_vocels);

    for (int i = 0; i < num_vocels; ++i) {
        auto coords = get_coordinates(i, num_vocels, vocel_points_per_layer);

        shiftX = std::get<0>(coords);
        shiftY = std::get<1>(coords) - 100;
        shiftZ = std::get<2>(coords) + 10;

        auto effectorPosition = std::make_shared<Position>(shiftX, shiftY, shiftZ);
        auto effector = std::make_shared<Effector>(effectorPosition);
        effector->initialise();
        vocalOutputs.emplace_back(effector);

        // Connect the effector to neurons across all clusters
        if (i > 0 && i % 17 == 0) {
            // Calculate the neuron index to connect to
            int neuronIndex = (i + num_vocels) % allNeurons.size();
            auto neuron = allNeurons[neuronIndex];

            // Adjust positions for connection
            PositionPtr currentSynapticGapPosition = neuron->getSoma()->getAxonHillock()->getAxon()
                    ->getAxonBouton()->getSynapticGap()->getPosition();
            PositionPtr currentEffectorPosition = effector->getPosition();

            newPositionX = currentEffectorPosition->x - 0.4;
            newPositionY = currentEffectorPosition->y - 0.4;
            newPositionZ = currentEffectorPosition->z - 0.4;

            currentSynapticGapPosition->x = newPositionX;
            currentSynapticGapPosition->y = newPositionY;
            currentSynapticGapPosition->z = newPositionZ;

            PositionPtr currentAxonBoutonPosition = neuron->getSoma()->getAxonHillock()->getAxon()
                    ->getAxonBouton()->getPosition();

            newPositionX += 0.4;
            newPositionY += 0.4;
            newPositionZ += 0.4;

            currentAxonBoutonPosition->x = newPositionX;
            currentAxonBoutonPosition->y = newPositionY;
            currentAxonBoutonPosition->z = newPositionZ;

            PositionPtr currentAxonPosition = neuron->getSoma()->getAxonHillock()->getAxon()->getPosition();

            newPositionX += 0.4;
            newPositionY += 0.4;
            newPositionZ += 0.4;

            currentAxonPosition->x = newPositionX;
            currentAxonPosition->y = newPositionY;
            currentAxonPosition->z = newPositionZ;

            neuron->getSoma()->getAxonHillock()->getAxon()->getAxonBouton()->getSynapticGap()->setAsAssociated();
        }
    }

    std::cout << "Created " << vocalOutputs.size() << " effectors." << std::endl;

    // Associate neurons between clusters
#pragma omp parallel for schedule(dynamic)
    for (size_t c1 = 0; c1 < clusters.size(); ++c1) {
        auto &cluster1 = clusters[c1];
        for (size_t c2 = c1 + 1; c2 < clusters.size(); ++c2) {
            auto &cluster2 = clusters[c2];
            // Associate neurons between clusters
            for (const auto &neuron1: cluster1->getNeurons()) {
                for (const auto &neuron2: cluster2->getNeurons()) {
                    associateSynapticGap(*neuron1, *neuron2, proximityThreshold);
                }
            }
        }
    }

    // Prepare to compute propagation rates using threads
    const size_t numThreads = std::thread::hardware_concurrency();
    std::vector<std::thread> threads(numThreads);

// Declare a mutex and the totalPropagationRate variable
    std::mutex totalPropagationRateMutex;
    double totalPropagationRate = 0.0;

// Calculate the number of neurons per thread
    size_t totalNeurons = allNeurons.size();
    size_t neuronsPerThread = totalNeurons / numThreads;
    size_t remainingNeurons = totalNeurons % numThreads;

// Launch threads to compute propagation rates
    size_t start = 0;
    for (size_t t = 0; t < numThreads; ++t) {
        size_t end = start + neuronsPerThread + (t < remainingNeurons ? 1 : 0);
        threads[t] = std::thread([start, end, &allNeurons, &totalPropagationRate, &totalPropagationRateMutex]() {
            double localSum = 0.0;
            for (size_t i = start; i < end; ++i) {
                double propagationRate = computePropagationRate(allNeurons[i]);
                localSum += propagationRate;
            }
            // Protect the addition with a mutex
            {
                std::lock_guard<std::mutex> lock(totalPropagationRateMutex);
                totalPropagationRate += localSum;
            }
        });
        start = end;
    }

// Join all threads
    for (auto &t: threads) {
        t.join();
    }

    std::cout << "The total propagation rate is " << totalPropagationRate << std::endl;

    // Perform batch insertion only if database is available
    if (useDatabase && conn_ptr) {
        try {
            pqxx::work txn(*conn_ptr);
            batch_insert_clusters(txn, clusters);
            std::cout << "Batch insertion completed." << std::endl;
            std::cout << "Total Propagation Rate: " << totalPropagationRate << std::endl;
            if (totalPropagationRate != 0) {
                txn.commit();
            } else {
                std::cout << "Propagation rate is zero. Aborting transaction." << std::endl;
                throw std::runtime_error("The propagation rate is not valid. Skipping database insertion.");
            }
        } catch (const std::exception &e) {
            std::cerr << "[ERROR] Database error during initial insert: " << e.what() << std::endl;
            // No need to abort explicitly; destructor of txn rolls back
        }
    }

    // Initialise and start the SensoryReceptorServer
    receptorServer.registerReceptors("Auditory_Left", auditoryReceptors[0]);
    receptorServer.registerReceptors("Auditory_Right", auditoryReceptors[1]);
//    std::vector<std::shared_ptr<SensoryReceptor>> &bladderBowelReceptors;
//    receptorServer.registerReceptors("BladderBowel", bladderBowelReceptors);
//    receptorServer.registerReceptors("Chemoreception", chemoreceptionReceptors);
//    receptorServer.registerReceptors("Electroception", electroceptionReceptors);
//    receptorServer.registerReceptors("Gustatory", gustatoryReceptors);
//    receptorServer.registerReceptors("HeartbeatRespiration", heartbeatRespirationReceptors);
//    receptorServer.registerReceptors("HungerThirst", hungerThirstReceptors);
//    receptorServer.registerReceptors("Interoceptive", interoceptiveReceptors);
//    receptorServer.registerReceptors("Lust", lustReceptors);
//    receptorServer.registerReceptors("Magnetoception", magnetoceptionReceptors);
    receptorServer.registerReceptors("Olfactory_Left", olfactoryReceptors[0]);
    receptorServer.registerReceptors("Olfactory_Right", olfactoryReceptors[1]);
//    receptorServer.registerReceptors("Pressure", pressureReceptors);
//    receptorServer.registerReceptors("Proprioceptive", proprioceptiveReceptors);
//    receptorServer.registerReceptors("Pruriceptive", pruriceptiveReceptors);
//    receptorServer.registerReceptors("Satiety", satietyReceptors);
//    receptorServer.registerReceptors("Somatosensory", somatosensoryReceptors);
//    receptorServer.registerReceptors("Stretch", stretchReceptors);
//    receptorServer.registerReceptors("Thermoception", thermoceptionReceptors);
    receptorServer.registerReceptors("Visual_Left", visualReceptors[0]);
    receptorServer.registerReceptors("Visual_Right", visualReceptors[1]);

    if (!receptorServer.startServer()) {
        std::cerr << "[WARNING] Failed to start SensoryReceptor server. Continuing without sensory server." << std::endl;
    }

    // Start threads for input and database updates (if applicable)
    //std::thread nvThread(runInteractor, std::ref(neurons), std::ref(neuron_mutex), std::ref(emptyAuditoryQueue), 0);
    //std::thread avThread(runInteractor, std::ref(emptyNeurons), std::ref(empty_neuron_mutex), std::ref(audioQueue), 1);
    std::thread inputThread(checkForQuit);
    std::thread clusterUpdateThread(updateClusters, std::ref(clusters), std::ref(running));
    std::thread dbThread;
    if (useDatabase && conn_ptr_updates) {
        // Launch the database-update thread only if database is available
        dbThread = std::thread(updateDatabase, std::ref(*conn_ptr_updates), std::ref(clusters));
    }

    // Main loop
    while (running) {
        /* compute delta time */
        double deltaTime = 0.1; // Placeholder for actual delta time calculation

#pragma omp parallel
        {
            // Auditory Receptors
#pragma omp for collapse(2) nowait
            for (size_t h = 0; h < auditoryReceptors.size(); ++h) {
                for (size_t i = 0; i < auditoryReceptors[h].size(); ++i) {
                    auditoryReceptors[h][i]->update(deltaTime);
                }
            }

            // BladderBowel Receptors
//#pragma omp for nowait
//            for (size_t i = 0; i < bladderBowelReceptors.size(); ++i) {
//                bladderBowelReceptors[i]->update(deltaTime);
//            }

            // Chemoreception Receptors
//#pragma omp for nowait
//            for (size_t i = 0; i < chemoreceptionReceptors.size(); ++i) {
//                chemoreceptionReceptors[i]->update(deltaTime);
//            }

            // Electroception Receptors
//#pragma omp for nowait
//            for (size_t i = 0; i < electroceptionReceptors.size(); ++i) {
//                electroceptionReceptors[i]->update(deltaTime);
//            }

            // Gustatory Receptors
//#pragma omp for nowait
//            for (size_t i = 0; i < gustatoryReceptors.size(); ++i) {
//                gustatoryReceptors[i]->update(deltaTime);
//            }

            // HeartbeatRespiration Receptors
//#pragma omp for nowait
//            for (size_t i = 0; i < heartbeatRespirationReceptors.size(); ++i) {
//                heartbeatRespirationReceptors[i]->update(deltaTime);
//            }

            // HungerThirst Receptors
//#pragma omp for nowait
//            for (size_t i = 0; i < hungerThirstReceptors.size(); ++i) {
//                hungerThirstReceptors[i]->update(deltaTime);
//            }

            // Interoceptive Receptors
//#pragma omp for nowait
//            for (size_t i = 0; i < interoceptiveReceptors.size(); ++i) {
//                interoceptiveReceptors[i]->update(deltaTime);
//            }

            // Lust Receptors
//#pragma omp for nowait
//            for (size_t i = 0; i < lustReceptors.size(); ++i) {
//                lustReceptors[i]->update(deltaTime);
//            }

            // Magnetoception Receptors
//#pragma omp for nowait
//            for (size_t i = 0; i < magnetoceptionReceptors.size(); ++i) {
//                magnetoceptionReceptors[i]->update(deltaTime);
//            }

            // Olfactory Receptors
#pragma omp for collapse(2) nowait
            for (size_t h = 0; h < olfactoryReceptors.size(); ++h) {
                for (size_t i = 0; i < olfactoryReceptors[h].size(); ++i) {
                    olfactoryReceptors[h][i]->update(deltaTime);
                }
            }

            // Pressure Receptors
//#pragma omp for nowait
//            for (size_t i = 0; i < pressureReceptors.size(); ++i) {
//                pressureReceptors[i]->update(deltaTime);
//            }

            // Propprioceptive Receptors
//#pragma omp for nowait
//            for (size_t i = 0; i < proprioceptiveReceptors.size(); ++i) {
//                proprioceptiveReceptors[i]->update(deltaTime);
//            }

            // Pruriceptive Receptors
//#pragma omp for nowait
//            for (size_t i = 0; i < pruriceptiveReceptors.size(); ++i) {
//                pruriceptiveReceptors[i]->update(deltaTime);
//            }

            // Satiety Receptors
//#pragma omp for nowait
//            for (size_t i = 0; i < satietyReceptors.size(); ++i) {
//                satietyReceptors[i]->update(deltaTime);
//            }

            // Somatosensory Receptors
//#pragma omp for nowait
//            for (size_t i = 0; i < somatosensoryReceptors.size(); ++i) {
//                somatosensoryReceptors[i]->update(deltaTime);
//            }

            // Stretch Receptors
//#pragma omp for nowait
//            for (size_t i = 0; i < stretchReceptors.size(); ++i) {
//                stretchReceptors[i]->update(deltaTime);
//            }

            // Thermoception Receptors
//#pragma omp for nowait
//            for (size_t i = 0; i < thermoceptionReceptors.size(); ++i) {
//                thermoceptionReceptors[i]->update(deltaTime);
//            }

            // Visual Receptors
#pragma omp for collapse(2) nowait
            for (size_t h = 0; h < visualReceptors.size(); ++h) {
                for (size_t i = 0; i < visualReceptors[h].size(); ++i) {
                    visualReceptors[h][i]->update(deltaTime);
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // Clean up
        //nvThread.join();
        //avThread.join();
        //mic->micStop();
        //micThread.join();
        // Once running == false, join all threads
        if (inputThread.joinable()) {
            inputThread.join();
        }
        if (clusterUpdateThread.joinable()) {
            clusterUpdateThread.join();
        }
        if (dbThread.joinable()) {
            // Signal updateDatabase thread to exit if needed
            {
                std::lock_guard<std::mutex> lock(changedNeuronsMutex);
                dbUpdateReady = true;
                conn_ptr.reset(); // Reset connection pointer to close the connection
                conn_ptr_updates.reset(); // Reset updates connection pointer
            }
            cv.notify_all();
            dbThread.join();
        }

        return 0;
    }
}