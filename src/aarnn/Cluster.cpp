#include "Cluster.h"
#include "Neuron.h"
#include <iostream>
#include <cmath>
#include <thread>
#include <mutex>
#include <atomic>
#include <omp.h>
#include <random>

// Initialize static member
int Cluster::nextClusterId = 0;
std::vector<std::shared_ptr<Position>> Cluster::existingClusterPositions;

// Constructor
Cluster::Cluster(const std::shared_ptr<Position>& position)
        : NeuronalComponent(position), clusterId(nextClusterId++)
{
    // Add the cluster's position to the list of existing cluster positions
    existingClusterPositions.push_back(position);
}

// Static method to create a new cluster
std::shared_ptr<Cluster> Cluster::createCluster(double minDistance)
{
    std::shared_ptr<Position> position = generateClusterPosition(minDistance);
    auto cluster = std::make_shared<Cluster>(position);
    return cluster;
}

// Static method to generate a position for a new cluster
std::shared_ptr<Position> Cluster::generateClusterPosition(double minDistance)
{
    // Generate a position that is at least minDistance away from all existing clusters
    std::shared_ptr<Position> newPosition;

    const int maxAttempts = 1000;
    int attempt = 0;
    bool positionFound = false;

    // Define the bounds within which to generate positions
    double minX = -1000.0, maxX = 1000.0;
    double minY = -1000.0, maxY = 1000.0;
    double minZ = -1000.0, maxZ = 1000.0;

    static std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<double> distX(minX, maxX);
    std::uniform_real_distribution<double> distY(minY, maxY);
    std::uniform_real_distribution<double> distZ(minZ, maxZ);

    while (attempt < maxAttempts && !positionFound) {
        attempt++;

        // Generate random coordinates
        double x = distX(rng);
        double y = distY(rng);
        double z = distZ(rng);

        newPosition = std::make_shared<Position>(x, y, z);

        // Check distance to existing clusters
        positionFound = true; // Assume it's valid unless we find a cluster too close
        for (const auto& pos : existingClusterPositions) {
            double dx = pos->x - x;
            double dy = pos->y - y;
            double dz = pos->z - z;
            double distance = sqrt(dx*dx + dy*dy + dz*dz);
            if (distance < minDistance) {
                positionFound = false;
                break;
            }
        }
    }

    if (!positionFound) {
        throw std::runtime_error("Could not find a suitable position for new cluster after maximum attempts");
    }

    // Position will be added to existingClusterPositions in the constructor
    return newPosition;
}

// Initialise the Cluster
void Cluster::initialise(int create_new_neurons, int neuron_points_per_layer, double proximityThreshold)
{
    NeuronalComponent::initialise(); // Call base class initialise

    if (!instanceInitialised) {
        std::cout << "Initialising Cluster with ID: " << clusterId << std::endl;

        // Create neurons
        createNeurons(create_new_neurons, neuron_points_per_layer);

        // Initialize each neuron in parallel
        size_t num_neurons = neurons.size();

        // Use OpenMP for parallel processing
#pragma omp parallel for schedule(dynamic)
        for (size_t i = 0; i < num_neurons; ++i) {
            auto& neuron = neurons[i];
            if (neuron) {
                // Optional: Protect console output with a mutex if needed
                // std::lock_guard<std::mutex> lock(consoleMutex);
                // std::cout << "Cluster initializing Neuron with ID: " << neuron->getNeuronId() << std::endl;

                neuron->initialise();
                neuron->updateFromCluster(std::static_pointer_cast<Cluster>(shared_from_this()));
                neuron->setPropagationRate(1.0); // Set default propagation rate
            }
        }

        // Associate neurons
        associateNeurons(proximityThreshold);
        instanceInitialised = true; // Mark as initialised
        std::cout << "Cluster initialised" << std::endl;
    }
}

void Cluster::createNeurons(int num_neurons, int neuron_points_per_layer)
{
    neurons.resize(num_neurons); // Resize to allow indexed access

    // Use OpenMP for parallel processing
#pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < num_neurons; ++i)
    {
        auto coords = get_coordinates(i, num_neurons, neuron_points_per_layer);

        double x = position->x + std::get<0>(coords);
        double y = position->y + std::get<1>(coords);
        double z = position->z + std::get<2>(coords);

        auto neuronPosition = std::make_shared<Position>(x, y, z);
        auto neuron = std::make_shared<Neuron>(neuronPosition);

        neurons[i] = neuron; // Assign neuron to its position in the vector
    }
}

// Associate neurons
void Cluster::associateNeurons(double proximityThreshold)
{
    size_t num_neurons = neurons.size();

    // Use OpenMP for parallel processing
#pragma omp parallel for schedule(dynamic)
    for (size_t i = 0; i < num_neurons; ++i)
    {
        for (size_t j = i + 1; j < num_neurons; ++j)
        {
            associateSynapticGap(*neurons[i], *neurons[j], proximityThreshold);
        }
    }
}

// Add a neuron to the cluster
void Cluster::addNeuron(const std::shared_ptr<Neuron>& neuron)
{
    if (neuron)
    {
        std::lock_guard<std::mutex> lock(neuronMutex);
        neurons.push_back(neuron);
        std::cout << "Neuron with ID: " << neuron->getNeuronId() << " added to Cluster ID: " << clusterId << std::endl;
    }
}

// Get the list of neurons
std::vector<std::shared_ptr<Neuron>> Cluster::getNeurons() const
{
    return neurons;
}

// Set the propagation rate
void Cluster::setPropagationRate(double rate)
{
    propagationRate = rate;
}

// Get the propagation rate
double Cluster::getPropagationRate() const
{
    return propagationRate;
}

// Set the Cluster ID
void Cluster::setClusterId(int id)
{
    clusterId = id;
}

// Get the Cluster ID
int Cluster::getClusterId() const
{
    return clusterId;
}

// Set the Cluster type
void Cluster::setClusterType(int type)
{
    clusterType = type;
}

// Get the Cluster type
int Cluster::getClusterType() const
{
    return clusterType;
}

// Update the Cluster state over time
void Cluster::update(double deltaTime)
{
    // Update energy levels
    updateEnergy(deltaTime);

    // Update each Neuron in parallel
    size_t num_neurons = neurons.size();

    // Use OpenMP for parallel processing
#pragma omp parallel for schedule(dynamic)
    for (size_t i = 0; i < num_neurons; ++i)
    {
        auto& neuron = neurons[i];
        if (neuron)
        {
            neuron->update(deltaTime);
        }
    }

    // Additional updates if necessary
}