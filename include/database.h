// database.h (UPDATED)

#ifndef AARNN_DATABASE_H
#define AARNN_DATABASE_H

#include <pqxx/pqxx> // For database interaction
#include <string>     // For std::string
#include <vector>     // For std::vector
#include <memory>     // For std::shared_ptr
#include <functional> // For std::function
#include <atomic>     // For std::atomic
#include <mutex>      // For std::mutex
#include <condition_variable> // For std::condition_variable

// Forward declarations for your AARNN entity classes.
// These are essential to avoid circular dependencies and allow shared_ptr usage.
// Ensure these classes have public `getId()`, `setId()`, `getPosition()`, and
// `get<Parent>()->getId()` methods, as well as necessary data getters.
class Cluster;
class Neuron;
class Soma;
class AxonHillock;
class Axon;
class AxonBouton;
class SynapticGap;
class AxonBranch;
class DendriteBranch;
class Dendrite;
class DendriteBouton;
class Position; // Assuming Position is a simple struct/class with x,y,z members

// Global atomic flags and mutexes, externed from main.cpp.
// These are used for thread synchronization in updateDatabase.
extern std::atomic<bool> running;
extern std::atomic<bool> dbUpdateReady;
extern std::mutex changedNeuronsMutex;
extern std::condition_variable cv;

// --- Public Database API Functions ---

/**
 * @brief Initializes the database schema by dropping existing tables and recreating them.
 *
 * This function executes DDL (Data Definition Language) statements to create tables
 * for clusters, neurons, and all their sub-components, exactly as defined in the
 * original schema. It will drop existing tables first.
 *
 * @param conn The active pqxx::connection to the PostgreSQL database.
 */
void initialise_database(pqxx::connection& conn);

/**
 * @brief Prepares all SQL INSERT and UPDATE statements for later efficient reuse.
 *
 * This function calls `conn.prepare()` for every DML statement (both INSERT and UPDATE),
 * optimizing subsequent `EXECUTE` calls in `batch_insert_clusters`, `updateDatabase`,
 * and the recursive insertion helpers. This significantly reduces database overhead.
 * This function should be called once when the application connects to the database.
 *
 * @param conn The active pqxx::connection to the PostgreSQL database.
 */
void prepareAllStatements(pqxx::connection& conn);

/**
 * @brief Performs a batch insertion of initial AARNN cluster structures and their hierarchies.
 *
 * This function iterates through the provided clusters and their nested components
 * (neurons, somas, axons, dendrites, etc.) and inserts them into the database.
 * It utilizes prepared SQL statements (`txn.exec_prepared()`) for all insertions,
 * which greatly improves performance compared to unprepared statements. The original
 * recursive insertion logic for axon and dendrite branches is preserved and also
 * benefits from prepared statements.
 *
 * @param txn The active pqxx::transaction_base to use for insertion.
 * @param clusters A vector of shared pointers to top-level Cluster objects.
 */
void batch_insert_clusters(pqxx::transaction_base& txn,
                           const std::vector<std::shared_ptr<Cluster>>& clusters);

/**
 * @brief Continuously updates neuron-related data in the database using a pipeline.
 *
 * This function runs in a loop, typically in a separate thread, waiting for a
 * `dbUpdateReady` signal. When triggered, it constructs and executes a `pqxx::pipeline`
 * of update statements for all relevant components within the clusters, optimizing
 * database writes by batching multiple queries into a single network round trip.
 * It uses the prepared statements defined in `prepareAllStatements`.
 *
 * @param conn The active pqxx::connection to the PostgreSQL database.
 * @param clusters A constant reference to the vector of top-level Cluster objects.
 */
void updateDatabase(pqxx::connection& conn,
                    const std::vector<std::shared_ptr<Cluster>>& clusters);

// --- Internal Helper Functions (for recursive insertions, matching original) ---

/**
 * @brief Recursively inserts AxonBranch entities and their sub-branches.
 *
 * This function is designed to match the original recursive insertion logic for
 * `axonbranches`. It inserts the current branch and then recursively calls
 * itself for any child axon branches within the current branch's axons.
 * It uses prepared statements for efficiency.
 *
 * @param txn The active pqxx::transaction_base.
 * @param branch A shared pointer to the current AxonBranch to insert.
 * @param parentId The ID of the parent (either an Axon's ID or another AxonBranch's ID).
 * @param parentIsAxon True if the immediate parent is an Axon, false if it's another AxonBranch.
 * This flag determines which foreign key (`parent_axon_id` or `parent_axon_branch_id`)
 * is populated in the `axonbranches` table.
 */
void insertAxonBranches(
        pqxx::transaction_base& txn,
        const std::shared_ptr<AxonBranch>& branch,
        int parentId,
        bool parentIsAxon
);

/**
 * @brief Recursively inserts DendriteBranch entities, their contained Dendrites, and sub-branches.
 *
 * This function is designed to match the original recursive insertion logic for
 * `dendritebranches`, `dendrites`, and `dendriteboutons`. It inserts the
 * current branch, then inserts all dendrites within it (and their optional
 * dendrite boutons), and then recursively calls itself for any child dendrite
 * branches within those dendrites. It uses prepared statements for efficiency.
 *
 * @param txn The active pqxx::transaction_base.
 * @param branch A shared pointer to the current DendriteBranch to insert.
 * @param parentId The ID of the parent (either a Soma's ID or a Dendrite's ID).
 * @param parentIsSoma True if the immediate parent is a Soma, false if it's a Dendrite.
 * This flag determines which foreign key (`soma_id` or `dendrite_id`)
 * is populated in the `dendritebranches` table.
 */
void insertDendriteBranches(
        pqxx::transaction_base& txn,
        const std::shared_ptr<DendriteBranch>& branch,
        int parentId,
        bool parentIsSoma
);

#endif // AARNN_DATABASE_H