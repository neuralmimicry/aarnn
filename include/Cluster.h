#ifndef CLUSTER_H
#define CLUSTER_H

#include "NeuronalComponent.h"
#include "Position.h"
#include "Neuron.h"
#include <vector>
#include <memory>
#include <mutex>

/**
 * @brief The Cluster class represents a group of Neurons.
 */
class Cluster : public NeuronalComponent
{
public:
    /**
     * @brief Creates a new cluster at a position that is at least minDistance away from existing clusters.
     * @param minDistance The minimum distance from existing clusters.
     * @return A shared pointer to the new Cluster.
     */
    static std::shared_ptr<Cluster> createCluster(double minDistance = 100.0);
    /**
     * @brief Constructor for the Cluster class.
     * @param position The position of the cluster in space.
     */
    explicit Cluster(const std::shared_ptr<Position>& position);

    /**
     * @brief Initializes the cluster and its neurons.
     */
    void initialise(int num_neurons, int neuron_points_per_layer, double proximityThreshold);

    /**
     * @brief Updates the cluster's state over time.
     * @param deltaTime The time elapsed since the last update.
     */
    void update(double deltaTime);

    void createNeurons(int num_neurons, int neuron_points_per_layer);
    void associateNeurons(double proximityThreshold);

    /**
     * @brief Adds a neuron to the cluster.
     * @param neuron A shared pointer to the neuron to add.
     */
    void addNeuron(const std::shared_ptr<Neuron>& neuron);

    /**
     * @brief Retrieves all neurons in the cluster.
     * @return A vector of shared pointers to the neurons.
     */
    std::vector<std::shared_ptr<Neuron>> getNeurons() const;

    /**
     * @brief Sets the propagation rate for the cluster.
     * @param rate The propagation rate to set.
     */
    void setPropagationRate(double rate);

    /**
     * @brief Gets the current propagation rate of the cluster.
     * @return The current propagation rate.
     */
    double getPropagationRate() const;

    /**
     * @brief Sets the cluster ID.
     * @param id The cluster ID to set.
     */
    void setClusterId(int id);

    /**
     * @brief Gets the cluster ID.
     * @return The cluster ID.
     */
    int getClusterId() const;

    /**
     * @brief Sets the cluster type.
     * @param type The cluster type to set.
     */
    void setClusterType(int type);

    /**
     * @brief Gets the cluster type.
     * @return The cluster type.
     */
    int getClusterType() const;

private:
    /**
      * @brief Generates a position for a new cluster that is at least minDistance away from existing clusters.
      * @param minDistance The minimum distance from existing clusters.
      * @return A shared pointer to the generated Position.
      */
    static std::shared_ptr<Position> generateClusterPosition(double minDistance);

    // Static members
    static int nextClusterId;     ///< Static counter for generating unique cluster IDs.
    static std::vector<std::shared_ptr<Position>> existingClusterPositions; ///< List of existing cluster positions.

    int clusterId;                ///< Unique identifier for the cluster.
    int clusterType;              ///< Type identifier for the cluster.
    double propagationRate;       ///< Propagation rate of signals within the cluster.

    std::vector<std::shared_ptr<Neuron>> neurons; ///< Collection of neurons within the cluster.
    std::mutex neuronMutex; ///< Mutex for thread safety when accessing neurons.

    bool instanceInitialised = false; ///< Flag to check if the cluster has been initialized.
};

#endif // CLUSTER_H
