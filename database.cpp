// database.cpp (UPDATED)

#include "database.h"
#include <pqxx/pqxx>
#include <iostream>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <vector>
#include <sstream>
#include <map>

#include "Cluster.h"
#include "Neuron.h"
#include "Soma.h"
#include "AxonHillock.h"
#include "Axon.h"
#include "AxonBouton.h"
#include "SynapticGap.h"
#include "AxonBranch.h"
#include "DendriteBranch.h"
#include "Dendrite.h"
#include "DendriteBouton.h"
#include "Position.h"

// Global atomic flags and mutexes, externed from main.cpp
extern std::atomic<bool> running;
extern std::atomic<bool> dbUpdateReady;
extern std::mutex changedNeuronsMutex;
extern std::condition_variable cv;

// --- DDL and Schema Initialization ---

/**
 * @brief Initializes the database schema by dropping existing tables and recreating them.
 *
 * This function executes DDL (Data Definition Language) statements to create tables
 * for clusters, neurons, and all their sub-components. It first drops tables to
 * ensure a clean slate for schema creation, exactly matching the original logic.
 *
 * @param conn The active pqxx::connection to the PostgreSQL database.
 */
void initialise_database(pqxx::connection& conn) {
    try {
        // Drop tables first to ensure a clean slate
        pqxx::work txn_drop{conn};
        txn_drop.exec("DROP TABLE IF EXISTS dendriteboutons, dendrites, dendritebranches, "
                      "synapticgaps, axonboutons, axons, axonbranches, axonhillocks, "
                      "somas, neurons, clusters CASCADE;");
        txn_drop.commit();
        std::cout << "[DB INIT] Existing tables dropped.\n";

        // Create tables
        pqxx::work txn_create{conn};
        txn_create.exec(
                "CREATE TABLE clusters (cluster_id SERIAL PRIMARY KEY, x REAL NOT NULL, y REAL NOT NULL, z REAL NOT NULL, propagation_rate REAL, cluster_type INTEGER, energy_level REAL NOT NULL, max_energy_level REAL NOT NULL);"
                "CREATE TABLE neurons  (neuron_id SERIAL PRIMARY KEY, cluster_id INTEGER, x REAL NOT NULL, y REAL NOT NULL, z REAL NOT NULL, propagation_rate REAL, neuron_type INTEGER, energy_level REAL NOT NULL, max_energy_level REAL NOT NULL);"
                "CREATE TABLE somas    (soma_id SERIAL PRIMARY KEY, neuron_id INTEGER, x REAL NOT NULL, y REAL NOT NULL, z REAL NOT NULL, energy_level REAL NOT NULL, max_energy_level REAL NOT NULL);"
                "CREATE TABLE axonhillocks (axon_hillock_id SERIAL PRIMARY KEY, soma_id INTEGER, x REAL NOT NULL, y REAL NOT NULL, z REAL NOT NULL, energy_level REAL NOT NULL, max_energy_level REAL NOT NULL);"
                "CREATE TABLE axonbranches (axon_branch_id SERIAL PRIMARY KEY, parent_axon_id INTEGER, parent_axon_branch_id INTEGER, x REAL NOT NULL, y REAL NOT NULL, z REAL NOT NULL, energy_level REAL NOT NULL, max_energy_level REAL NOT NULL);"
                "CREATE TABLE axons     (axon_id SERIAL PRIMARY KEY, axon_hillock_id INTEGER, axon_branch_id INTEGER, x REAL NOT NULL, y REAL NOT NULL, z REAL NOT NULL, energy_level REAL NOT NULL, max_energy_level REAL NOT NULL);"
                "CREATE TABLE axonboutons (axon_bouton_id SERIAL PRIMARY KEY, axon_id INTEGER, x REAL NOT NULL, y REAL NOT NULL, z REAL NOT NULL, energy_level REAL NOT NULL, max_energy_level REAL NOT NULL);"
                "CREATE TABLE synapticgaps (synaptic_gap_id SERIAL PRIMARY KEY, axon_bouton_id INTEGER, x REAL NOT NULL, y REAL NOT NULL, z REAL NOT NULL, energy_level REAL NOT NULL, max_energy_level REAL NOT NULL);"
                "CREATE TABLE dendritebranches (dendrite_branch_id SERIAL PRIMARY KEY, soma_id INTEGER, parent_dendrite_id INTEGER, x REAL NOT NULL, y REAL NOT NULL, z REAL NOT NULL, energy_level REAL NOT NULL, max_energy_level REAL NOT NULL);"
                "CREATE TABLE dendrites (dendrite_id SERIAL PRIMARY KEY, dendrite_branch_id INTEGER, x REAL NOT NULL, y REAL NOT NULL, z REAL NOT NULL, energy_level REAL NOT NULL, max_energy_level REAL NOT NULL);"
                "CREATE TABLE dendriteboutons (dendrite_bouton_id SERIAL PRIMARY KEY, dendrite_id INTEGER UNIQUE, x REAL NOT NULL, y REAL NOT NULL, z REAL NOT NULL, energy_level REAL NOT NULL, max_energy_level REAL NOT NULL);"
        );
        // Add foreign key constraints
        txn_create.exec(
                "ALTER TABLE neurons        ADD FOREIGN KEY (cluster_id)       REFERENCES clusters(cluster_id);"
                "ALTER TABLE somas          ADD FOREIGN KEY (neuron_id)        REFERENCES neurons(neuron_id);"
                "ALTER TABLE axonhillocks   ADD FOREIGN KEY (soma_id)          REFERENCES somas(soma_id);"
                "ALTER TABLE axons          ADD FOREIGN KEY (axon_hillock_id)  REFERENCES axonhillocks(axon_hillock_id);" // Can be NULL for branched axons
                "ALTER TABLE axons          ADD FOREIGN KEY (axon_branch_id)   REFERENCES axonbranches(axon_branch_id);" // Can be NULL for main axons
                "ALTER TABLE axonbranches   ADD FOREIGN KEY (parent_axon_id)        REFERENCES axons(axon_id),"
                "                            ADD FOREIGN KEY (parent_axon_branch_id) REFERENCES axonbranches(axon_branch_id);"
                "ALTER TABLE axonboutons    ADD FOREIGN KEY (axon_id)          REFERENCES axons(axon_id);"
                "ALTER TABLE synapticgaps   ADD FOREIGN KEY (axon_bouton_id)   REFERENCES axonboutons(axon_bouton_id);"
                "ALTER TABLE dendritebranches ADD FOREIGN KEY (soma_id)        REFERENCES somas(soma_id)," // Can be NULL for non-top branches
                "                             ADD FOREIGN KEY (parent_dendrite_id)    REFERENCES dendrites(dendrite_id);" // Can be NULL for main branches to soma
                "ALTER TABLE dendrites       ADD FOREIGN KEY (dendrite_branch_id) REFERENCES dendritebranches(dendrite_branch_id);"
                "ALTER TABLE dendriteboutons ADD FOREIGN KEY (dendrite_id)     REFERENCES dendrites(dendrite_id);"
        );
        txn_create.commit();
        std::cout << "[DB INIT] Schema created.\n";
    } catch (const pqxx::sql_error& e) {
        std::cerr << "[DB INIT ERROR] SQL error during schema creation: " << e.what()
                  << " (SQLSTATE: " << e.sqlstate() << ")\n";
        throw; // Re-throw to indicate a critical failure
    } catch (const std::exception& e) {
        std::cerr << "[DB INIT ERROR] General error during schema creation: " << e.what() << "\n";
        throw; // Re-throw to indicate a critical failure
    }
}

// --- Prepared Statement Definitions for ALL DML (INSERT & UPDATE) ---

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
void prepareAllStatements(pqxx::connection& conn) {
    // --- UPDATE Statements (from original code) ---
    conn.prepare("update_cluster",     "UPDATE clusters     SET x=$1,y=$2,z=$3,propagation_rate=$4,energy_level=$5,max_energy_level=$6 WHERE cluster_id=$7");
    conn.prepare("update_neuron",      "UPDATE neurons      SET x=$1,y=$2,z=$3,propagation_rate=$4,energy_level=$5,max_energy_level=$6 WHERE neuron_id=$7");
    conn.prepare("update_soma",        "UPDATE somas        SET x=$1,y=$2,z=$3,energy_level=$4,max_energy_level=$5 WHERE soma_id=$6");
    conn.prepare("update_axonhillock", "UPDATE axonhillocks SET x=$1,y=$2,z=$3,energy_level=$4,max_energy_level=$5 WHERE axon_hillock_id=$6");
    conn.prepare("update_axon",        "UPDATE axons        SET x=$1,y=$2,z=$3,energy_level=$4,max_energy_level=$5 WHERE axon_id=$6");
    conn.prepare("update_axonbouton",  "UPDATE axonboutons  SET x=$1,y=$2,z=$3,energy_level=$4,max_energy_level=$5 WHERE axon_bouton_id=$6");
    conn.prepare("update_synapticgap", "UPDATE synapticgaps SET x=$1,y=$2,z=$3,energy_level=$4,max_energy_level=$5 WHERE synaptic_gap_id=$6");
    conn.prepare("update_axonbranch",  "UPDATE axonbranches SET x=$1,y=$2,z=$3,energy_level=$4,max_energy_level=$5 WHERE axon_branch_id=$6");
    conn.prepare("update_dendritebranch","UPDATE dendritebranches SET x=$1,y=$2,z=$3,energy_level=$4,max_energy_level=$5 WHERE dendrite_branch_id=$6");
    conn.prepare("update_dendrite",    "UPDATE dendrites    SET x=$1,y=$2,z=$3,energy_level=$4,max_energy_level=$5 WHERE dendrite_id=$6");
    conn.prepare("update_dendritebtn","UPDATE dendriteboutons SET x=$1,y=$2,z=$3,energy_level=$4,max_energy_level=$5 WHERE dendrite_bouton_id=$6");

    // --- INSERT Statements (for batch_insert_clusters and recursive helpers) ---
    conn.prepare("insert_cluster",
                 "INSERT INTO clusters (x,y,z,propagation_rate,cluster_type,energy_level,max_energy_level) "
                 "VALUES ($1,$2,$3,$4,$5,$6,$7) RETURNING cluster_id");
    conn.prepare("insert_neuron",
                 "INSERT INTO neurons (cluster_id,x,y,z,propagation_rate,neuron_type,energy_level,max_energy_level) "
                 "VALUES ($1,$2,$3,$4,$5,$6,$7,$8) RETURNING neuron_id");
    conn.prepare("insert_soma",
                 "INSERT INTO somas (neuron_id,x,y,z,energy_level,max_energy_level) "
                 "VALUES ($1,$2,$3,$4,$5,$6) RETURNING soma_id");
    conn.prepare("insert_axonhillock",
                 "INSERT INTO axonhillocks (soma_id,x,y,z,energy_level,max_energy_level) "
                 "VALUES ($1,$2,$3,$4,$5,$6) RETURNING axon_hillock_id");

    // For Axons, handling the `axon_branch_id` as potentially NULL for main axons
    conn.prepare("insert_axon",
                 "INSERT INTO axons (axon_hillock_id,axon_branch_id,x,y,z,energy_level,max_energy_level) "
                 "VALUES ($1,$2,$3,$4,$5,$6,$7) RETURNING axon_id");

    conn.prepare("insert_axonbouton",
                 "INSERT INTO axonboutons (axon_id,x,y,z,energy_level,max_energy_level) "
                 "VALUES ($1,$2,$3,$4,$5,$6) RETURNING axon_bouton_id");
    conn.prepare("insert_synapticgap",
                 "INSERT INTO synapticgaps (axon_bouton_id,x,y,z,energy_level,max_energy_level) "
                 "VALUES ($1,$2,$3,$4,$5,$6) RETURNING synaptic_gap_id");

    // Prepared statements for recursive AxonBranch insertion.
    // The original code used one SQL string with a conditional for `parent_axon_id` or `parent_axon_branch_id`.
    // Prepared statements require explicit definition for each variant.
    conn.prepare("insert_axonbranch_from_axon",
                 "INSERT INTO axonbranches (parent_axon_id,parent_axon_branch_id,x,y,z,energy_level,max_energy_level) "
                 "VALUES ($1,NULL,$2,$3,$4,$5,$6) RETURNING axon_branch_id");
    conn.prepare("insert_axonbranch_from_branch",
                 "INSERT INTO axonbranches (parent_axon_id,parent_axon_branch_id,x,y,z,energy_level,max_energy_level) "
                 "VALUES (NULL,$1,$2,$3,$4,$5,$6) RETURNING axon_branch_id");

    // Prepared statements for recursive DendriteBranch insertion.
    // Similar to axon branches, handled with two explicit prepared statements.
    conn.prepare("insert_dendritebranch_from_soma",
                 "INSERT INTO dendritebranches (soma_id,parent_dendrite_id,x,y,z,energy_level,max_energy_level) "
                 "VALUES ($1,NULL,$2,$3,$4,$5,$6) RETURNING dendrite_branch_id");
    conn.prepare("insert_dendritebranch_from_dendrite",
                 "INSERT INTO dendritebranches (soma_id,parent_dendrite_id,x,y,z,energy_level,max_energy_level) "
                 "VALUES (NULL,$1,$2,$3,$4,$5,$6) RETURNING dendrite_branch_id");

    conn.prepare("insert_dendrite",
                 "INSERT INTO dendrites (dendrite_branch_id,x,y,z,energy_level,max_energy_level) "
                 "VALUES ($1,$2,$3,$4,$5,$6) RETURNING dendrite_id");
    conn.prepare("insert_dendritebouton",
                 "INSERT INTO dendriteboutons (dendrite_id,x,y,z,energy_level,max_energy_level) "
                 "VALUES ($1,$2,$3,$4,$5,$6) RETURNING dendrite_bouton_id");

    std::cout << "[DB INIT] All INSERT and UPDATE statements prepared.\n";
}

// --- Helper: build EXECUTE call (Identical to original) ---

/**
 * @brief Constructs an SQL string for executing a prepared statement with parameters.
 *
 * This variadic template function takes the prepared statement name and arguments,
 * then formats them into an `EXECUTE` SQL command string. It properly quotes
 * string arguments using `txn.quote()`. This is used by `pqxx::pipeline`.
 *
 * @tparam Args Variadic template arguments for the parameters of the prepared statement.
 * @param txn The active pqxx::transaction_base to use for quoting.
 * @param name The name of the prepared statement.
 * @param args The parameters to pass to the prepared statement.
 * @return A string containing the `EXECUTE` SQL command.
 */
template<typename... Args>
std::string make_execute(pqxx::transaction_base& txn, const char* name, Args const&... args) {
    std::ostringstream os;
    os << "EXECUTE " << name << "(";
    auto quoted = std::vector<std::string>{txn.quote(args)...};
    for (size_t i = 0; i < quoted.size(); ++i) {
        if (i) os << ',';
        os << quoted[i];
    }
    os << ");";
    return os.str();
}

// --- Pipelined updates (Optimized using prepared statements and pipeline) ---

/**
 * @brief Continuously updates neuron-related data in the database using a pipeline.
 *
 * This function runs in a separate thread (implied by `running` and `cv`).
 * It waits for a signal (`dbUpdateReady`) indicating that data is ready for update.
 * Once signaled, it acquires a lock, resets the flag, and then uses a `pqxx::pipeline`
 * to efficiently send multiple `UPDATE` statements to the database within a single
 * transaction. This significantly reduces network round trips compared to individual
 * `exec_params` calls. The pipeline uses the prepared statements defined in
 * `prepareAllStatements`.
 *
 * @param conn The active pqxx::connection to the PostgreSQL database.
 * @param clusters A constant reference to the vector of top-level Cluster objects.
 */
void updateDatabase(pqxx::connection& conn,
                    const std::vector<std::shared_ptr<Cluster>>& clusters) {
    // `prepareAllStatements` must be called once at application startup.
    // Do not call it inside this loop, as it would be inefficient.

    while (running) {
        std::unique_lock<std::mutex> lk(changedNeuronsMutex);
        // Wait for a signal that updates are ready, or if the application is shutting down.
        cv.wait(lk, []{ return dbUpdateReady.load() || !running.load(); });

        // If not running and no pending update, gracefully exit the loop.
        if (!running.load() && !dbUpdateReady.load()) {
            std::cout << "[DB UPDATE] Shutting down update thread.\n";
            break;
        }

        dbUpdateReady = false; // Reset the flag once updates are ready to be processed.
        lk.unlock(); // Release lock before performing database operations to avoid blocking other threads.

        try {
            pqxx::work txn{conn};
            pqxx::pipeline pipe{txn};

            // Recursively add all entities' update statements to the pipeline
            for (auto const& c : clusters) {
                if (!c) continue; // Skip null clusters

                // Cluster Update
                pipe.insert(make_execute(txn, "update_cluster",
                                         c->getPosition()->x,
                                         c->getPosition()->y,
                                         c->getPosition()->z,
                                         c->getPropagationRate(),
                                         c->getEnergyLevel(),
                                         c->getMaxEnergyLevel(),
                                         c->getClusterId()));

                // Iterate through Neurons and their components
                for (auto const& n : c->getNeurons()) {
                    if (!n) continue; // Skip null neurons

                    // Neuron Update
                    pipe.insert(make_execute(txn, "update_neuron",
                                             n->getPosition()->x,
                                             n->getPosition()->y,
                                             n->getPosition()->z,
                                             n->getPropagationRate(),
                                             n->getEnergyLevel(),
                                             n->getMaxEnergyLevel(),
                                             n->getNeuronId()));
                    auto s = n->getSoma();
                    if (s) {
                        // Soma Update
                        pipe.insert(make_execute(txn, "update_soma",
                                                 s->getPosition()->x,
                                                 s->getPosition()->y,
                                                 s->getPosition()->z,
                                                 s->getEnergyLevel(),
                                                 s->getMaxEnergyLevel(),
                                                 s->getSomaId()));
                        auto ah = s->getAxonHillock();
                        if (ah) {
                            // AxonHillock Update
                            pipe.insert(make_execute(txn, "update_axonhillock",
                                                     ah->getPosition()->x,
                                                     ah->getPosition()->y,
                                                     ah->getPosition()->z,
                                                     ah->getEnergyLevel(),
                                                     ah->getMaxEnergyLevel(),
                                                     ah->getAxonHillockId()));
                            auto ax = ah->getAxon();
                            if (ax) {
                                // Axon Update
                                pipe.insert(make_execute(txn, "update_axon",
                                                         ax->getPosition()->x,
                                                         ax->getPosition()->y,
                                                         ax->getPosition()->z,
                                                         ax->getEnergyLevel(),
                                                         ax->getMaxEnergyLevel(),
                                                         ax->getAxonId()));
                                auto btn = ax->getAxonBouton();
                                if (btn) {
                                    // AxonBouton Update
                                    pipe.insert(make_execute(txn, "update_axonbouton",
                                                             btn->getPosition()->x,
                                                             btn->getPosition()->y,
                                                             btn->getPosition()->z,
                                                             btn->getEnergyLevel(),
                                                             btn->getMaxEnergyLevel(),
                                                             btn->getAxonBoutonId()));
                                    auto gap = btn->getSynapticGap();
                                    if (gap) {
                                        // SynapticGap Update
                                        pipe.insert(make_execute(txn, "update_synapticgap",
                                                                 gap->getPosition()->x,
                                                                 gap->getPosition()->y,
                                                                 gap->getPosition()->z,
                                                                 gap->getEnergyLevel(),
                                                                 gap->getMaxEnergyLevel(),
                                                                 gap->getSynapticGapId()));
                                    }
                                }
                                // Iterate and update AxonBranches recursively
                                // Note: For updates, the recursive functions are not called here
                                // as they are designed for initial *insertion*.
                                // Instead, we directly pipeline updates for all collected branches.
                                // This requires a flattened list of all branches to update.
                                // For simplicity and to match the original structure, I'll iterate through branches.
                                for (auto const& br : ax->getAxonBranches()) {
                                    // AxonBranch Update
                                    pipe.insert(make_execute(txn, "update_axonbranch",
                                                             br->getPosition()->x,
                                                             br->getPosition()->y,
                                                             br->getPosition()->z,
                                                             br->getEnergyLevel(),
                                                             br->getMaxEnergyLevel(),
                                                             br->getAxonBranchId()));
                                    // Recursively add updates for sub-branches within the axon branches.
                                    // This part of the update loop structure is implied by the original
                                    // insertion recursion, but requires traversing the C++ object graph
                                    // to find all existing axon branches to update.
                                    // The original `updateDatabase` only iterated the top-level branches
                                    // from axon. It didn't explicitly recurse for updates on sub-branches.
                                    // To maintain consistency, I'll update only the direct branches.
                                    // A full recursive update would involve more complex traversal here.
                                    // If sub-branches can change position/energy, they also need to be updated.
                                    // Assuming getAxons() on AxonBranch provides further elements for traversal:
                                    // (This is a simplified approach, a more robust solution might pre-collect all updatable objects)
                                    for (auto const& sub_ax : br->getAxons()) {
                                        // If these 'sub_ax' are also stored in DB, they'd need updates.
                                        // For now, only the branch itself is updated.
                                        // If `sub_ax->getAxonBranches()` means more levels, those need to be updated.
                                        // To truly match original update depth, a recursive update helper is needed.
                                        // For now, sticking to the single level of branch update in original `updateDatabase`.
                                    }
                                }
                            }
                        }
                        // Iterate and update DendriteBranches recursively
                        for (auto const& br : s->getDendriteBranches()) {
                            // DendriteBranch Update
                            pipe.insert(make_execute(txn, "update_dendritebranch",
                                                     br->getPosition()->x,
                                                     br->getPosition()->y,
                                                     br->getPosition()->z,
                                                     br->getEnergyLevel(),
                                                     br->getMaxEnergyLevel(),
                                                     br->getDendriteBranchId()));
                            // Iterate and update Dendrites within the branch
                            for (auto const& d : br->getDendrites()) {
                                // Dendrite Update
                                pipe.insert(make_execute(txn, "update_dendrite",
                                                         d->getPosition()->x,
                                                         d->getPosition()->y,
                                                         d->getPosition()->z,
                                                         d->getEnergyLevel(),
                                                         d->getMaxEnergyLevel(),
                                                         d->getDendriteId()));
                                auto btn = d->getDendriteBouton();
                                if (btn) {
                                    // DendriteBouton Update
                                    pipe.insert(make_execute(txn, "update_dendritebtn",
                                                             btn->getPosition()->x,
                                                             btn->getPosition()->y,
                                                             btn->getPosition()->z,
                                                             btn->getEnergyLevel(),
                                                             btn->getMaxEnergyLevel(),
                                                             btn->getDendriteBoutonId()));
                                }
                                // Similar to AxonBranches, if sub-dendrite branches exist, they need update.
                                // Original `updateDatabase` loop only went one level deep for DendriteBranches.
                                // Assuming for updates, direct children are sufficient.
                                for (auto const& sub_br : d->getDendriteBranches()) {
                                    pipe.insert(make_execute(txn, "update_dendritebranch",
                                                             sub_br->getPosition()->x,
                                                             sub_br->getPosition()->y,
                                                             sub_br->getPosition()->z,
                                                             sub_br->getEnergyLevel(),
                                                             sub_br->getMaxEnergyLevel(),
                                                             sub_br->getDendriteBranchId()));
                                }
                            }
                        }
                    }
                }
            }
            pipe.complete(); // Send all batched queries to the database
            txn.commit();    // Commit the transaction to make changes permanent
            std::cout << "[DB UPDATE] Pipeline executed and transaction committed.\n";
        } catch (const std::exception& e) {
            std::cerr << "[DB UPDATE ERROR] " << e.what() << "\n";
            // Transaction is automatically aborted on exception by pqxx::work destructor
        }
    }
}

// --- Batch Insert Clusters & Sub-components (Optimized with prepared statements) ---

/**
 * @brief Performs a batch insertion of initial AARNN cluster structures and their hierarchies.
 *
 * This function iterates through the provided clusters and their nested components
 * (neurons, somas, axons, dendrites, etc.) and inserts them into the database.
 * It utilizes prepared SQL statements (`txn.exec_prepared()`) for all insertions,
 * which greatly improves performance compared to unprepared statements by reducing
 * query parsing overhead on the database server. The original recursive insertion
 * logic for axon and dendrite branches is preserved and also benefits from prepared statements.
 *
 * @param txn The active pqxx::transaction_base to use for insertion.
 * @param clusters A vector of shared pointers to top-level Cluster objects.
 */
void batch_insert_clusters(pqxx::transaction_base& txn,
                           const std::vector<std::shared_ptr<Cluster>>& clusters) {

    std::cout << "[INFO] Starting batch insertion for clusters and components using prepared statements.\n";

    for (auto const& c : clusters) {
        if (!c) continue; // Skip null clusters

        // Insert Cluster
        // Using prepared statement "insert_cluster" defined in prepareAllStatements
        auto cr = txn.exec_prepared("insert_cluster",
                                    c->getPosition()->x, c->getPosition()->y, c->getPosition()->z,
                                    c->getPropagationRate(), c->getClusterType(), // Assuming getClusterType returns int
                                    c->getEnergyLevel(), c->getMaxEnergyLevel());
        int cid = cr[0][0].as<int>(); // Retrieve the generated cluster_id
        c->setClusterId(cid); // Set the ID back into the C++ object

        // Iterate and insert Neurons within this Cluster
        for (auto const& n : c->getNeurons()) {
            // Original code had `!n || !n->getSoma()`. We need a soma for neuron insertion to make sense.
            if (!n || !n->getSoma()) {
                std::cerr << "[WARNING] Skipping null Neuron or Neuron with null Soma in cluster " << cid << ".\n";
                continue;
            }
            // Insert Neuron
            // Using prepared statement "insert_neuron"
            auto nr = txn.exec_prepared("insert_neuron",
                                        cid, n->getPosition()->x, n->getPosition()->y, n->getPosition()->z,
                                        n->getPropagationRate(), n->getNeuronType(), // Assuming NeuronType is integer
                                        n->getEnergyLevel(), n->getMaxEnergyLevel());
            int nid = nr[0][0].as<int>(); // Retrieve generated neuron_id
            n->setNeuronId(nid); // Set the ID back into the C++ object

            auto s = n->getSoma(); // Already checked for null above
            // Insert Soma
            // Using prepared statement "insert_soma"
            auto sr = txn.exec_prepared("insert_soma",
                                        nid, s->getPosition()->x, s->getPosition()->y, s->getPosition()->z,
                                        s->getEnergyLevel(), s->getMaxEnergyLevel());
            int sid = sr[0][0].as<int>(); // Retrieve generated soma_id
            s->setSomaId(sid); // Set the ID back into the C++ object

            // Handle AxonHillock, Axon, Bouton, SynapticGap
            auto ah = s->getAxonHillock();
            if (ah) {
                // Insert AxonHillock
                // Using prepared statement "insert_axonhillock"
                auto ahr = txn.exec_prepared("insert_axonhillock",
                                             sid, ah->getPosition()->x, ah->getPosition()->y, ah->getPosition()->z,
                                             ah->getEnergyLevel(), ah->getMaxEnergyLevel());
                int ahid = ahr[0][0].as<int>(); // Retrieve generated axon_hillock_id
                ah->setAxonHillockId(ahid); // Set the ID back into the C++ object

                auto ax = ah->getAxon();
                if (ax) {
                    // Insert main Axon (from AxonHillock). axon_branch_id is NULL here.
                    // Using prepared statement "insert_axon"
                    auto axr = txn.exec_prepared("insert_axon",
                                                 ahid,
                                                 std::optional<int>{}, // Corrected: use an empty std::optional for NULL int
                                                 ax->getPosition()->x, ax->getPosition()->y, ax->getPosition()->z,
                                                 ax->getEnergyLevel(), ax->getMaxEnergyLevel());
                    int axid = axr[0][0].as<int>(); // Retrieve generated axon_id
                    ax->setAxonId(axid); // Set the ID back into the C++ object

                    auto btn = ax->getAxonBouton();
                    if (btn) {
                        // Insert AxonBouton
                        // Using prepared statement "insert_axonbouton"
                        auto btr = txn.exec_prepared("insert_axonbouton",
                                                     axid,
                                                     btn->getPosition()->x, btn->getPosition()->y, btn->getPosition()->z,
                                                     btn->getEnergyLevel(), btn->getMaxEnergyLevel());
                        btn->setAxonBoutonId(btr[0][0].as<int>()); // Set generated axon_bouton_id

                        auto gap = btn->getSynapticGap();
                        if (gap) {
                            // Insert SynapticGap
                            // Using prepared statement "insert_synapticgap"
                            auto sgr = txn.exec_prepared("insert_synapticgap",
                                                         btn->getAxonBoutonId(), // Foreign key to axon_bouton_id
                                                         gap->getPosition()->x, gap->getPosition()->y, gap->getPosition()->z,
                                                         gap->getEnergyLevel(), gap->getMaxEnergyLevel());
                            gap->setSynapticGapId(sgr[0][0].as<int>()); // Set generated synaptic_gap_id
                        }
                    }
                    // Recursively insert AxonBranches starting from this Axon
                    // Pass `axid` as parentId and `true` indicating parent is an Axon.
                    for (auto const& br : ax->getAxonBranches()) {
                        insertAxonBranches(txn, br, axid, true);
                    }
                }
            }
            // Recursively insert DendriteBranches starting from this Soma
            // Pass `sid` as parentId and `true` indicating parent is a Soma.
            for (auto const& db : s->getDendriteBranches()) {
                insertDendriteBranches(txn, db, sid, true);
            }
        }
    }
    std::cout << "[INFO] All clusters and their components inserted using prepared statements.\n";
}

// --- Implementation: recursive AxonBranch insertion (Optimized with Prepared Statements) ---
/**
 * @brief Recursively inserts AxonBranch entities and their sub-branches.
 *
 * This function adheres to the original recursive logic but utilizes prepared
 * statements for efficiency. It inserts the current branch into `axonbranches`
 * (linking to either an Axon or another AxonBranch parent) and then recursively
 * calls itself for any child axon branches within the current branch's axons.
 *
 * @param txn The active pqxx::transaction_base.
 * @param branch A shared pointer to the current AxonBranch to insert.
 * @param parentId The ID of the parent (either an Axon's ID or another AxonBranch's ID).
 * @param parentIsAxon True if the immediate parent is an Axon, false if another AxonBranch.
 * This flag determines which foreign key (`parent_axon_id` or `parent_axon_branch_id`)
 * is populated in the `axonbranches` table.
 */
void insertAxonBranches(
        pqxx::transaction_base& txn,
        const std::shared_ptr<AxonBranch>& branch,
        int parentId,
        bool parentIsAxon // true if parent is an Axon, false if another AxonBranch
) {
    if (!branch) return;

    int bid; // The ID of the newly inserted AxonBranch
    pqxx::result r;

    // Choose the correct prepared statement based on the parent type
    if (parentIsAxon) {
        // Parent is an Axon: use parent_axon_id, parent_axon_branch_id is NULL
        r = txn.exec_prepared("insert_axonbranch_from_axon",
                              parentId, // parent_axon_id
                              branch->getPosition()->x,
                              branch->getPosition()->y,
                              branch->getPosition()->z,
                              branch->getEnergyLevel(),
                              branch->getMaxEnergyLevel());
    } else {
        // Parent is another AxonBranch: use parent_axon_branch_id, parent_axon_id is NULL
        r = txn.exec_prepared("insert_axonbranch_from_branch",
                              parentId, // parent_axon_branch_id
                              branch->getPosition()->x,
                              branch->getPosition()->y,
                              branch->getPosition()->z,
                              branch->getEnergyLevel(),
                              branch->getMaxEnergyLevel());
    }

    bid = r[0][0].as<int>(); // Retrieve the generated axon_branch_id
    branch->setAxonBranchId(bid); // Set the ID back into the C++ object

    // Recursively insert children branches if they exist within the axons that are part of this branch.
    // The schema dictates that AxonBranches contain Axons, and those Axons can have further AxonBranches.
    for (auto const& ax_in_branch : branch->getAxons()) {
        // Insert this Axon. Its parent is the current AxonBranch (bid).
        // This means setting the `axon_branch_id` column in the `axons` table.
        // `axon_hillock_id` will be NULL for these (or should be handled if an axon can belong to both).
        // Assuming axons found here are solely children of branches and not hillocks.
        // If an Axon can be part of both a hillock and a branch, the schema needs clarification or a different design.
        // For now, consistent with `insert_axon` prepared statement, it allows `axon_hillock_id` or `axon_branch_id` to be NULL.

        // This is a critical point: The original code's `batch_insert_clusters`
        // inserted `axons` only from `axonhillocks`. The recursive `insertAxonBranches`
        // then iterated `ax->getAxonBranches()`. It *did not* insert `ax` itself.
        // If `branch->getAxons()` can contain *new* `Axon` objects that need inserting,
        // then those need to be inserted here, setting their `axon_branch_id`.
        // If `branch->getAxons()` merely refers to already inserted Axons (e.g., the main one from hillock),
        // then this loop might be for traversing for *its* branches.
        // Given the prompt: "The following is the original complete code. Your answer changed the database schema but that cannot happen due to outside dependencies. Try again."
        // And the original `insertAxonBranches` recursive part: `for (auto const& ax : branch->getAxons()) { for (auto const& sub : ax->getAxonBranches()) { insertAxonBranches(txn, sub, bid, false); } }`
        // This means `ax` objects inside `branch->getAxons()` are *not* themselves inserted here,
        // but are used to find *their* `AxonBranches` (sub-branches) for further recursion.
        // This implies that `axons` table only gets populated with main axons from hillocks initially,
        // and that `axon_branch_id` for an axon in `axons` might conceptually link an axon back to its *parent* branch,
        // if it originated from a branch. This is an unusual FK setup.
        // I will stick to the original logic: `ax` itself is not inserted here, only its sub-branches.

        for (auto const& ax_in_branch : branch->getAxons()) {
            if (!ax_in_branch) continue;
            // The `ax_in_branch` object here is an Axon. We need to ensure it has an ID
            // if its axon_id is used as a foreign key in any subsequent branch.
            // If it's a new axon within a branch, it should be inserted with its parent_axon_branch_id.
            // However, the current schema `axons (axon_hillock_id, axon_branch_id)` and
            // `axonbranches (parent_axon_id, parent_axon_branch_id)` suggests Axon objects
            // can be related to *either* a hillock *or* a branch, but not necessarily nested
            // as child Axons within AxonBranches in a way that requires insertion *here*.
            // The original code only inserts the `ax` once from the hillock.
            // So, `ax_in_branch` here likely refers to an *already inserted* `Axon` object (the main axon itself, or another)
            // whose branches are being traversed.

            // To ensure the recursion matches the original, we only recurse on the AxonBranches of `ax_in_branch`.
            for (auto const& sub_branch : ax_in_branch->getAxonBranches()) {
                // The parent of `sub_branch` is now the `AxonBranch` identified by `bid`.
                // So, `parentIsAxon` is `false`.
                insertAxonBranches(txn, sub_branch, bid, false);
            }
        }
    }
}

// --- Implementation: recursive DendriteBranch insertion (Optimized with Prepared Statements) ---
/**
 * @brief Recursively inserts DendriteBranch entities, their contained Dendrites, and sub-branches.
 *
 * This function adheres to the original recursive logic but utilizes prepared
 * statements for efficiency. It inserts the current branch into `dendritebranches`
 * (linking to either a Soma or a Dendrite parent), then inserts its `Dendrite` children
 * (each with their optional `DendriteBouton`), and finally recursively calls itself
 * for any child dendrite branches within those dendrites.
 *
 * @param txn The active pqxx::transaction_base.
 * @param branch A shared pointer to the current DendriteBranch to insert.
 * @param parentId The ID of the parent (either a Soma's ID or a Dendrite's ID).
 * @param parentIsSoma True if the immediate parent is a Soma, false if it's a Dendrite.
 * This flag determines which foreign key (`soma_id` or `parent_dendrite_id`)
 * is populated in the `dendritebranches` table.
 */
void insertDendriteBranches(
        pqxx::transaction_base& txn,
        const std::shared_ptr<DendriteBranch>& branch,
        int parentId,
        bool parentIsSoma
) {
    if (!branch) return;

    int bid; // The ID of the newly inserted DendriteBranch
    pqxx::result r_branch;

    // Choose the correct prepared statement based on the parent type
    if (parentIsSoma) {
        // Parent is a Soma: use soma_id, parent_dendrite_id is NULL
        r_branch = txn.exec_prepared("insert_dendritebranch_from_soma",
                                     parentId, // soma_id
                                     branch->getPosition()->x,
                                     branch->getPosition()->y,
                                     branch->getPosition()->z,
                                     branch->getEnergyLevel(),
                                     branch->getMaxEnergyLevel());
    } else {
        // Parent is a Dendrite: use parent_dendrite_id, soma_id is NULL
        r_branch = txn.exec_prepared("insert_dendritebranch_from_dendrite",
                                     parentId, // parent_dendrite_id
                                     branch->getPosition()->x,
                                     branch->getPosition()->y,
                                     branch->getPosition()->z,
                                     branch->getEnergyLevel(),
                                     branch->getMaxEnergyLevel());
    }
    bid = r_branch[0][0].as<int>(); // Retrieve the generated dendrite_branch_id
    branch->setDendriteBranchId(bid); // Set the ID back into the C++ object

    // Insert child Dendrites that belong to this DendriteBranch
    for (auto const& d : branch->getDendrites()) {
        if (!d) continue; // Skip null dendrites

        int did; // The ID of the newly inserted Dendrite
        // Insert Dendrite
        // Using prepared statement "insert_dendrite"
        pqxx::result r_dendrite = txn.exec_prepared(
                "insert_dendrite",
                bid, // dendrite_branch_id (foreign key to this branch's ID)
                d->getPosition()->x,
                d->getPosition()->y,
                d->getPosition()->z,
                d->getEnergyLevel(),
                d->getMaxEnergyLevel());
        did = r_dendrite[0][0].as<int>(); // Retrieve generated dendrite_id
        d->setDendriteId(did); // Set the ID back into the C++ object

        // Insert DendriteBouton (if it exists) for this Dendrite
        auto btn = d->getDendriteBouton();
        if (btn) {
            // Insert DendriteBouton
            // Using prepared statement "insert_dendritebouton"
            pqxx::result r_bouton = txn.exec_prepared(
                    "insert_dendritebouton",
                    did, // dendrite_id (foreign key to this dendrite's ID)
                    btn->getPosition()->x,
                    btn->getPosition()->y,
                    btn->getPosition()->z,
                    btn->getEnergyLevel(),
                    btn->getMaxEnergyLevel());
            btn->setDendriteBoutonId(r_bouton[0][0].as<int>()); // Set generated dendrite_bouton_id
        }

        // Recursively insert sub-DendriteBranches that belong to this Dendrite
        // Pass `did` as parentId and `false` indicating parent is a Dendrite.
        for (auto const& sub_branch : d->getDendriteBranches()) {
            insertDendriteBranches(txn, sub_branch, did, false);
        }
    }
}