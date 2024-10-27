// database.cpp

#include "database.h"
#include <iostream>
#include <mutex>
#include <unordered_set>
#include <pqxx/pqxx>

// Global variables (ensure these are defined appropriately in your code)
extern std::atomic<bool> running;
extern std::atomic<bool> dbUpdateReady;
extern std::mutex changedNeuronsMutex;
extern std::condition_variable cv;

extern std::unordered_set<std::shared_ptr<Neuron>> changedNeurons;
extern std::unordered_set<std::shared_ptr<Cluster>> changedClusters;

// Forward declarations to resolve circular dependencies
void updateAxon(pqxx::work& txn, const std::shared_ptr<Axon>& axon);
void updateAxonBranch(pqxx::work& txn, const std::shared_ptr<AxonBranch>& axonBranch);
void updateDendriteBranch(pqxx::work& txn, const std::shared_ptr<DendriteBranch>& dendriteBranch);

void initialise_database(pqxx::connection& conn) {
    // Begin a transaction to drop existing tables
    {
        pqxx::work txn(conn);
        txn.exec("DROP TABLE IF EXISTS clusters, neurons, somas, axonhillocks, axons, axonboutons, synapticgaps, dendritebranches, dendrites, dendriteboutons, axonbranches CASCADE;");
        txn.commit();
    }

    // Begin a new transaction to create tables without foreign key constraints
    {
        pqxx::work txn(conn);
        // Create tables without foreign key constraints
        txn.exec(
                // Clusters Table
                "CREATE TABLE clusters ("
                "cluster_id SERIAL PRIMARY KEY,"
                "x REAL NOT NULL,"
                "y REAL NOT NULL,"
                "z REAL NOT NULL,"
                "propagation_rate REAL,"
                "cluster_type INTEGER,"
                "energy_level REAL NOT NULL"
                ");"
                // Neurons Table
                "CREATE TABLE neurons ("
                "neuron_id SERIAL PRIMARY KEY,"
                "cluster_id INTEGER,"
                "x REAL NOT NULL,"
                "y REAL NOT NULL,"
                "z REAL NOT NULL,"
                "propagation_rate REAL,"
                "neuron_type INTEGER,"
                "energy_level REAL NOT NULL"
                ");"
                // Somas Table
                "CREATE TABLE somas ("
                "soma_id SERIAL PRIMARY KEY,"
                "neuron_id INTEGER,"
                "x REAL NOT NULL,"
                "y REAL NOT NULL,"
                "z REAL NOT NULL,"
                "energy_level REAL NOT NULL"
                ");"
                // Axon Hillocks Table
                "CREATE TABLE axonhillocks ("
                "axon_hillock_id SERIAL PRIMARY KEY,"
                "soma_id INTEGER,"
                "x REAL NOT NULL,"
                "y REAL NOT NULL,"
                "z REAL NOT NULL,"
                "energy_level REAL NOT NULL"
                ");"
                // Axon Branches Table
                "CREATE TABLE axonbranches ("
                "axon_branch_id SERIAL PRIMARY KEY,"
                "parent_axon_id INTEGER,"
                "parent_axon_branch_id INTEGER,"
                "x REAL NOT NULL,"
                "y REAL NOT NULL,"
                "z REAL NOT NULL,"
                "energy_level REAL NOT NULL"
                ");"
                // Axons Table
                "CREATE TABLE axons ("
                "axon_id SERIAL PRIMARY KEY,"
                "axon_hillock_id INTEGER,"
                "axon_branch_id INTEGER,"
                "x REAL NOT NULL,"
                "y REAL NOT NULL,"
                "z REAL NOT NULL,"
                "energy_level REAL NOT NULL"
                ");"
                // Axon Boutons Table
                "CREATE TABLE axonboutons ("
                "axon_bouton_id SERIAL PRIMARY KEY,"
                "axon_id INTEGER,"
                "x REAL NOT NULL,"
                "y REAL NOT NULL,"
                "z REAL NOT NULL,"
                "energy_level REAL NOT NULL"
                ");"
                // Synaptic Gaps Table
                "CREATE TABLE synapticgaps ("
                "synaptic_gap_id SERIAL PRIMARY KEY,"
                "axon_bouton_id INTEGER,"
                "x REAL NOT NULL,"
                "y REAL NOT NULL,"
                "z REAL NOT NULL,"
                "energy_level REAL NOT NULL"
                ");"
                // Dendrite Branches Table
                "CREATE TABLE dendritebranches ("
                "dendrite_branch_id SERIAL PRIMARY KEY,"
                "soma_id INTEGER,"
                "dendrite_id INTEGER,"
                "x REAL NOT NULL,"
                "y REAL NOT NULL,"
                "z REAL NOT NULL,"
                "energy_level REAL NOT NULL"
                ");"
                // Dendrites Table
                "CREATE TABLE dendrites ("
                "dendrite_id SERIAL PRIMARY KEY,"
                "dendrite_branch_id INTEGER,"
                "x REAL NOT NULL,"
                "y REAL NOT NULL,"
                "z REAL NOT NULL,"
                "energy_level REAL NOT NULL"
                ");"
                // Dendrite Boutons Table
                "CREATE TABLE dendriteboutons ("
                "dendrite_bouton_id SERIAL PRIMARY KEY,"
                "dendrite_id INTEGER UNIQUE,"
                "x REAL NOT NULL,"
                "y REAL NOT NULL,"
                "z REAL NOT NULL,"
                "energy_level REAL NOT NULL"
                ");"
        );

        // Add foreign key constraints using ALTER TABLE
        txn.exec(
                // Neurons Table Foreign Key
                "ALTER TABLE neurons "
                "ADD FOREIGN KEY (cluster_id) REFERENCES clusters(cluster_id);"
                // Somas Table Foreign Key
                "ALTER TABLE somas "
                "ADD FOREIGN KEY (neuron_id) REFERENCES neurons(neuron_id);"
                // Axon Hillocks Table Foreign Key
                "ALTER TABLE axonhillocks "
                "ADD FOREIGN KEY (soma_id) REFERENCES somas(soma_id);"
                // Axons Table Foreign Keys
                "ALTER TABLE axons "
                "ADD FOREIGN KEY (axon_hillock_id) REFERENCES axonhillocks(axon_hillock_id), "
                "ADD FOREIGN KEY (axon_branch_id) REFERENCES axonbranches(axon_branch_id);"
                // Axon Branches Table Foreign Keys
                "ALTER TABLE axonbranches "
                "ADD FOREIGN KEY (parent_axon_id) REFERENCES axons(axon_id), "
                "ADD FOREIGN KEY (parent_axon_branch_id) REFERENCES axonbranches(axon_branch_id);"
                // Axon Boutons Table Foreign Key
                "ALTER TABLE axonboutons "
                "ADD FOREIGN KEY (axon_id) REFERENCES axons(axon_id);"
                // Synaptic Gaps Table Foreign Key
                "ALTER TABLE synapticgaps "
                "ADD FOREIGN KEY (axon_bouton_id) REFERENCES axonboutons(axon_bouton_id);"
                // Dendrite Branches Table Foreign Keys
                "ALTER TABLE dendritebranches "
                "ADD FOREIGN KEY (soma_id) REFERENCES somas(soma_id), "
                "ADD FOREIGN KEY (dendrite_id) REFERENCES dendrites(dendrite_id);"
                // Dendrites Table Foreign Key
                "ALTER TABLE dendrites "
                "ADD FOREIGN KEY (dendrite_branch_id) REFERENCES dendritebranches(dendrite_branch_id);"
                // Dendrite Boutons Table Foreign Key
                "ALTER TABLE dendriteboutons "
                "ADD FOREIGN KEY (dendrite_id) REFERENCES dendrites(dendrite_id);"
        );

        txn.commit();
        std::cout << "Created tables and added foreign key constraints successfully." << std::endl;
    }
}

void insertDendriteBranches(pqxx::transaction_base& txn, const std::shared_ptr<DendriteBranch>& dendriteBranch, int parent_id, bool parent_is_soma) {
    if (!dendriteBranch) {
        return;
    }

    int dendrite_branch_id;

    if (parent_is_soma) {
        // Insert Dendrite Branch connected to Soma
        pqxx::result branch_result = txn.exec_params(
                "INSERT INTO dendritebranches (soma_id, x, y, z, energy_level) VALUES ($1, $2, $3, $4, $5) RETURNING dendrite_branch_id",
                parent_id,
                dendriteBranch->getPosition()->x,
                dendriteBranch->getPosition()->y,
                dendriteBranch->getPosition()->z,
                dendriteBranch->getEnergyLevel()
        );
        dendrite_branch_id = branch_result[0][0].as<int>();
        dendriteBranch->setDendriteBranchId(dendrite_branch_id);
        std::cout << "Inserted Dendrite Branch ID: " << dendrite_branch_id << std::endl;
    } else {
        // Insert Dendrite Branch connected to Dendrite
        pqxx::result branch_result = txn.exec_params(
                "INSERT INTO dendritebranches (dendrite_id, x, y, z, energy_level) VALUES ($1, $2, $3, $4, $5) RETURNING dendrite_branch_id",
                parent_id,
                dendriteBranch->getPosition()->x,
                dendriteBranch->getPosition()->y,
                dendriteBranch->getPosition()->z,
                dendriteBranch->getEnergyLevel()
        );
        dendrite_branch_id = branch_result[0][0].as<int>();
        dendriteBranch->setDendriteBranchId(dendrite_branch_id);
        std::cout << "Inserted Dendrite Branch ID: " << dendrite_branch_id << std::endl;
    }

    // For each Dendrite in the Dendrite Branch
    for (const auto& dendrite : dendriteBranch->getDendrites()) {
        // Insert Dendrite and get dendrite_id
        pqxx::result dendrite_result = txn.exec_params(
                "INSERT INTO dendrites (dendrite_branch_id, x, y, z, energy_level) VALUES ($1, $2, $3, $4, $5) RETURNING dendrite_id",
                dendrite_branch_id,
                dendrite->getPosition()->x,
                dendrite->getPosition()->y,
                dendrite->getPosition()->z,
                dendrite->getEnergyLevel()
        );
        int dendrite_id = dendrite_result[0][0].as<int>();
        dendrite->setDendriteId(dendrite_id);
        std::cout << "Inserted Dendrite ID: " << dendrite_id << std::endl;

        // Insert Dendrite Bouton and get dendrite_bouton_id
        pqxx::result bouton_result = txn.exec_params(
                "INSERT INTO dendriteboutons (dendrite_id, x, y, z, energy_level) VALUES ($1, $2, $3, $4, $5) RETURNING dendrite_bouton_id",
                dendrite_id,
                dendrite->getDendriteBouton()->getPosition()->x,
                dendrite->getDendriteBouton()->getPosition()->y,
                dendrite->getDendriteBouton()->getPosition()->z,
                dendrite->getDendriteBouton()->getEnergyLevel()
        );
        int dendrite_bouton_id = bouton_result[0][0].as<int>();
        dendrite->getDendriteBouton()->setDendriteBoutonId(dendrite_bouton_id);
        std::cout << "Inserted Dendrite Bouton ID: " << dendrite_bouton_id << std::endl;

        // Recursively insert any further Dendrite Branches stemming from this Dendrite
        for (const auto& innerDendriteBranch : dendrite->getDendriteBranches()) {
            insertDendriteBranches(txn, innerDendriteBranch, dendrite_id, false);
        }
    }
}

void insertAxonBranches(pqxx::transaction_base& txn, const std::shared_ptr<AxonBranch>& axonBranch, int parent_id, bool parent_is_axon_hillock) {
    if (!axonBranch) {
        return;
    }

    int axon_branch_id;

    if (parent_is_axon_hillock) {
        // Insert Axon Branch connected to Axon Hillock
        pqxx::result branch_result = txn.exec_params(
                "INSERT INTO axonbranches (parent_axon_id, x, y, z, energy_level) VALUES ($1, $2, $3, $4, $5) RETURNING axon_branch_id",
                parent_id,
                axonBranch->getPosition()->x,
                axonBranch->getPosition()->y,
                axonBranch->getPosition()->z,
                axonBranch->getEnergyLevel()
        );
        axon_branch_id = branch_result[0][0].as<int>();
        axonBranch->setAxonBranchId(axon_branch_id);
        std::cout << "Inserted Axon Branch ID: " << axon_branch_id << std::endl;
    } else {
        // Insert Axon Branch connected to another Axon Branch
        pqxx::result branch_result = txn.exec_params(
                "INSERT INTO axonbranches (parent_axon_branch_id, x, y, z, energy_level) VALUES ($1, $2, $3, $4, $5) RETURNING axon_branch_id",
                parent_id,
                axonBranch->getPosition()->x,
                axonBranch->getPosition()->y,
                axonBranch->getPosition()->z,
                axonBranch->getEnergyLevel()
        );
        axon_branch_id = branch_result[0][0].as<int>();
        axonBranch->setAxonBranchId(axon_branch_id);
        std::cout << "Inserted Axon Branch ID: " << axon_branch_id << std::endl;
    }

    // For each Axon in the Axon Branch
    for (const auto& axon : axonBranch->getAxons()) {
        // Insert Axon and get axon_id
        pqxx::result axon_result = txn.exec_params(
                "INSERT INTO axons (axon_branch_id, x, y, z, energy_level) VALUES ($1, $2, $3, $4, $5) RETURNING axon_id",
                axon_branch_id,
                axon->getPosition()->x,
                axon->getPosition()->y,
                axon->getPosition()->z,
                axon->getEnergyLevel()
        );
        int axon_id = axon_result[0][0].as<int>();
        axon->setAxonId(axon_id);
        std::cout << "Inserted Axon ID: " << axon_id << std::endl;

        // Insert Axon Bouton and get axon_bouton_id
        pqxx::result bouton_result = txn.exec_params(
                "INSERT INTO axonboutons (axon_id, x, y, z, energy_level) VALUES ($1, $2, $3, $4, $5) RETURNING axon_bouton_id",
                axon_id,
                axon->getAxonBouton()->getPosition()->x,
                axon->getAxonBouton()->getPosition()->y,
                axon->getAxonBouton()->getPosition()->z,
                axon->getAxonBouton()->getEnergyLevel()
        );
        int axon_bouton_id = bouton_result[0][0].as<int>();
        axon->getAxonBouton()->setAxonBoutonId(axon_bouton_id);
        std::cout << "Inserted Axon Bouton ID: " << axon_bouton_id << std::endl;

        // Insert Synaptic Gap
        pqxx::result synaptic_gap_result = txn.exec_params(
                "INSERT INTO synapticgaps (axon_bouton_id, x, y, z, energy_level) VALUES ($1, $2, $3, $4, $5) RETURNING synaptic_gap_id",
                axon_bouton_id,
                axon->getAxonBouton()->getSynapticGap()->getPosition()->x,
                axon->getAxonBouton()->getSynapticGap()->getPosition()->y,
                axon->getAxonBouton()->getSynapticGap()->getPosition()->z,
                axon->getAxonBouton()->getSynapticGap()->getEnergyLevel()
        );
        int synaptic_gap_id = synaptic_gap_result[0][0].as<int>();
        axon->getAxonBouton()->getSynapticGap()->setSynapticGapId(synaptic_gap_id);
        std::cout << "Inserted Synaptic Gap ID: " << synaptic_gap_id << std::endl;

        // Recursively insert any further Axon Branches stemming from this Axon
        for (const auto& childAxonBranch : axon->getAxonBranches()) {
            insertAxonBranches(txn, childAxonBranch, axon_id, false);
        }
    }
}

void batch_insert_clusters(pqxx::transaction_base& txn, const std::vector<std::shared_ptr<Cluster>>& clusters) {
    std::cout << "Starting batch insertion of " << clusters.size() << " clusters." << std::endl;
    int outercount = 0;
    for (const auto& cluster : clusters) {
        outercount++;
        try {
            if (!cluster) {
                std::cerr << "Cluster " << outercount << " is null. Skipping insertion." << std::endl;
                continue;
            }

            // Insert Cluster and get cluster_id
            pqxx::result cluster_result = txn.exec_params(
                    "INSERT INTO clusters (x, y, z, propagation_rate, cluster_type, energy_level) VALUES ($1, $2, $3, $4, $5, $6) RETURNING cluster_id",
                    cluster->getPosition()->x,
                    cluster->getPosition()->y,
                    cluster->getPosition()->z,
                    cluster->getPropagationRate(),
                    cluster->getClusterType(),
                    cluster->getEnergyLevel()
            );
            int cluster_id = cluster_result[0][0].as<int>();
            cluster->setClusterId(cluster_id);
            std::cout << "Inserted Cluster ID: " << cluster_id << std::endl;

            if (cluster->getNeurons().empty()) {
                std::cerr << "Cluster " << outercount << " has no neurons. Skipping neuron insertion." << std::endl;
                continue;
            }

            int count = 0;
            for (const auto& neuron : cluster->getNeurons()) {
                count++;
                try {
                    if (!neuron) {
                        std::cerr << "Neuron " << count << " in cluster " << outercount << " is null. Skipping insertion." << std::endl;
                        continue;
                    }
                    if (!neuron->getSoma()) {
                        std::cerr << "Neuron " << count << " in cluster " << outercount << " has a null Soma. Skipping insertion." << std::endl;
                        continue;
                    }

                    // Insert Neuron and get neuron_id
                    pqxx::result neuron_result = txn.exec_params(
                            "INSERT INTO neurons (cluster_id, x, y, z, propagation_rate, neuron_type, energy_level) VALUES ($1, $2, $3, $4, $5, $6, $7) RETURNING neuron_id",
                            cluster_id,
                            neuron->getPosition()->x,
                            neuron->getPosition()->y,
                            neuron->getPosition()->z,
                            neuron->getPropagationRate(),
                            neuron->getNeuronType(),
                            neuron->getEnergyLevel()
                    );
                    int neuron_id = neuron_result[0][0].as<int>();
                    neuron->setNeuronId(neuron_id);
                    std::cout << "Inserted Neuron ID: " << neuron_id << std::endl;

                    // Insert Soma and get soma_id
                    pqxx::result soma_result = txn.exec_params(
                            "INSERT INTO somas (neuron_id, x, y, z, energy_level) VALUES ($1, $2, $3, $4, $5) RETURNING soma_id",
                            neuron_id,
                            neuron->getSoma()->getPosition()->x,
                            neuron->getSoma()->getPosition()->y,
                            neuron->getSoma()->getPosition()->z,
                            neuron->getSoma()->getEnergyLevel()
                    );
                    int soma_id = soma_result[0][0].as<int>();
                    neuron->getSoma()->setSomaId(soma_id);
                    std::cout << "Inserted Soma ID: " << soma_id << std::endl;

                    // Insert Axon Hillock and get axon_hillock_id
                    auto axonHillock = neuron->getSoma()->getAxonHillock();
                    if (axonHillock) {
                        pqxx::result axon_hillock_result = txn.exec_params(
                                "INSERT INTO axonhillocks (soma_id, x, y, z, energy_level) VALUES ($1, $2, $3, $4, $5) RETURNING axon_hillock_id",
                                soma_id,
                                axonHillock->getPosition()->x,
                                axonHillock->getPosition()->y,
                                axonHillock->getPosition()->z,
                                axonHillock->getEnergyLevel()
                        );
                        int axon_hillock_id = axon_hillock_result[0][0].as<int>();
                        axonHillock->setAxonHillockId(axon_hillock_id);
                        std::cout << "Inserted Axon Hillock ID: " << axon_hillock_id << std::endl;

                        // Insert Axon and get axon_id
                        auto axon = axonHillock->getAxon();
                        if (axon) {
                            pqxx::result axon_result = txn.exec_params(
                                    "INSERT INTO axons (axon_hillock_id, x, y, z, energy_level) VALUES ($1, $2, $3, $4, $5) RETURNING axon_id",
                                    axon_hillock_id,
                                    axon->getPosition()->x,
                                    axon->getPosition()->y,
                                    axon->getPosition()->z,
                                    axon->getEnergyLevel()
                            );
                            int axon_id = axon_result[0][0].as<int>();
                            axon->setAxonId(axon_id);
                            std::cout << "Inserted Axon ID: " << axon_id << std::endl;

                            // Insert Axon Bouton and get axon_bouton_id
                            auto axonBouton = axon->getAxonBouton();
                            if (axonBouton) {
                                pqxx::result axon_bouton_result = txn.exec_params(
                                        "INSERT INTO axonboutons (axon_id, x, y, z, energy_level) VALUES ($1, $2, $3, $4, $5) RETURNING axon_bouton_id",
                                        axon_id,
                                        axonBouton->getPosition()->x,
                                        axonBouton->getPosition()->y,
                                        axonBouton->getPosition()->z,
                                        axonBouton->getEnergyLevel()
                                );
                                int axon_bouton_id = axon_bouton_result[0][0].as<int>();
                                axonBouton->setAxonBoutonId(axon_bouton_id);
                                std::cout << "Inserted Axon Bouton ID: " << axon_bouton_id << std::endl;

                                // Insert Synaptic Gap
                                auto synapticGap = axonBouton->getSynapticGap();
                                if (synapticGap) {
                                    pqxx::result synaptic_gap_result = txn.exec_params(
                                            "INSERT INTO synapticgaps (axon_bouton_id, x, y, z, energy_level) VALUES ($1, $2, $3, $4, $5) RETURNING synaptic_gap_id",
                                            axon_bouton_id,
                                            synapticGap->getPosition()->x,
                                            synapticGap->getPosition()->y,
                                            synapticGap->getPosition()->z,
                                            synapticGap->getEnergyLevel()
                                    );
                                    int synaptic_gap_id = synaptic_gap_result[0][0].as<int>();
                                    synapticGap->setSynapticGapId(synaptic_gap_id);
                                    std::cout << "Inserted Synaptic Gap ID: " << synaptic_gap_id << std::endl;
                                }
                            }

                            // Insert Axon Branches recursively starting from the Axon Hillock's Axon
                            for (const auto& axonBranch : axon->getAxonBranches()) {
                                insertAxonBranches(txn, axonBranch, axon_id, false);
                            }
                        }
                    }

                    // Insert Dendrite Branches recursively
                    for (const auto& dendriteBranch : neuron->getSoma()->getDendriteBranches()) {
                        insertDendriteBranches(txn, dendriteBranch, soma_id, true);
                    }
                } catch (const std::exception& e) {
                    std::cerr << "Error inserting neuron " << count << " in cluster " << outercount << ": " << e.what() << std::endl;
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Error inserting cluster " << outercount << ": " << e.what() << std::endl;
        }
    }
}

void updateAxonBranch(pqxx::work& txn, const std::shared_ptr<AxonBranch>& axonBranch) {
    if (!axonBranch) return;

    double axon_branch_x = axonBranch->getPosition()->x;
    double axon_branch_y = axonBranch->getPosition()->y;
    double axon_branch_z = axonBranch->getPosition()->z;
    double axon_branch_energy_level = axonBranch->getEnergyLevel();
    int axon_branch_id = axonBranch->getAxonBranchId();

    txn.exec_params(
            "UPDATE axonbranches SET x = $1, y = $2, z = $3, energy_level = $4 WHERE axon_branch_id = $5",
            axon_branch_x, axon_branch_y, axon_branch_z, axon_branch_energy_level, axon_branch_id
    );

    // Update Axons in the Branch
    const auto& axons = axonBranch->getAxons();
    for (const auto& axon : axons) {
        updateAxon(txn, axon);
    }
}

void updateAxon(pqxx::work& txn, const std::shared_ptr<Axon>& axon) {
    if (!axon) return;

    double axon_x = axon->getPosition()->x;
    double axon_y = axon->getPosition()->y;
    double axon_z = axon->getPosition()->z;
    double axon_energy_level = axon->getEnergyLevel();
    int axon_id = axon->getAxonId();

    txn.exec_params(
            "UPDATE axons SET x = $1, y = $2, z = $3, energy_level = $4 WHERE axon_id = $5",
            axon_x, axon_y, axon_z, axon_energy_level, axon_id
    );

    // Update Axon Bouton
    auto axonBouton = axon->getAxonBouton();
    if (axonBouton) {
        double axon_bouton_x = axonBouton->getPosition()->x;
        double axon_bouton_y = axonBouton->getPosition()->y;
        double axon_bouton_z = axonBouton->getPosition()->z;
        double axon_bouton_energy_level = axonBouton->getEnergyLevel();
        int axon_bouton_id = axonBouton->getAxonBoutonId();

        txn.exec_params(
                "UPDATE axonboutons SET x = $1, y = $2, z = $3, energy_level = $4 WHERE axon_bouton_id = $5",
                axon_bouton_x, axon_bouton_y, axon_bouton_z, axon_bouton_energy_level, axon_bouton_id
        );

        // Update Synaptic Gap
        auto synapticGap = axonBouton->getSynapticGap();
        if (synapticGap) {
            double synaptic_gap_x = synapticGap->getPosition()->x;
            double synaptic_gap_y = synapticGap->getPosition()->y;
            double synaptic_gap_z = synapticGap->getPosition()->z;
            double synaptic_gap_energy_level = synapticGap->getEnergyLevel();
            int synaptic_gap_id = synapticGap->getSynapticGapId();

            txn.exec_params(
                    "UPDATE synapticgaps SET x = $1, y = $2, z = $3, energy_level = $4 WHERE synaptic_gap_id = $5",
                    synaptic_gap_x, synaptic_gap_y, synaptic_gap_z, synaptic_gap_energy_level, synaptic_gap_id
            );
        }
    }

    // Update Axon Branches
    const auto& axonBranches = axon->getAxonBranches();
    for (const auto& axonBranch : axonBranches) {
        updateAxonBranch(txn, axonBranch);
    }
}

void updateDendriteBranch(pqxx::work& txn, const std::shared_ptr<DendriteBranch>& dendriteBranch) {
    if (!dendriteBranch) return;

    double dendrite_branch_x = dendriteBranch->getPosition()->x;
    double dendrite_branch_y = dendriteBranch->getPosition()->y;
    double dendrite_branch_z = dendriteBranch->getPosition()->z;
    double dendrite_branch_energy_level = dendriteBranch->getEnergyLevel();
    int dendrite_branch_id = dendriteBranch->getDendriteBranchId();

    txn.exec_params(
            "UPDATE dendritebranches SET x = $1, y = $2, z = $3, energy_level = $4 WHERE dendrite_branch_id = $5",
            dendrite_branch_x, dendrite_branch_y, dendrite_branch_z, dendrite_branch_energy_level, dendrite_branch_id
    );

    // Update Dendrites in the Branch
    const auto& dendrites = dendriteBranch->getDendrites();
    for (const auto& dendrite : dendrites) {
        if (!dendrite) continue;

        double dendrite_x = dendrite->getPosition()->x;
        double dendrite_y = dendrite->getPosition()->y;
        double dendrite_z = dendrite->getPosition()->z;
        double dendrite_energy_level = dendrite->getEnergyLevel();
        int dendrite_id = dendrite->getDendriteId();

        txn.exec_params(
                "UPDATE dendrites SET x = $1, y = $2, z = $3, energy_level = $4 WHERE dendrite_id = $5",
                dendrite_x, dendrite_y, dendrite_z, dendrite_energy_level, dendrite_id
        );

        // Update Dendrite Bouton
        auto dendriteBouton = dendrite->getDendriteBouton();
        if (dendriteBouton) {
            double dendrite_bouton_x = dendriteBouton->getPosition()->x;
            double dendrite_bouton_y = dendriteBouton->getPosition()->y;
            double dendrite_bouton_z = dendriteBouton->getPosition()->z;
            double dendrite_bouton_energy_level = dendriteBouton->getEnergyLevel();
            int dendrite_bouton_id = dendriteBouton->getDendriteBoutonId();

            txn.exec_params(
                    "UPDATE dendriteboutons SET x = $1, y = $2, z = $3, energy_level = $4 WHERE dendrite_bouton_id = $5",
                    dendrite_bouton_x, dendrite_bouton_y, dendrite_bouton_z, dendrite_bouton_energy_level, dendrite_bouton_id
            );
        }

        // Recursively update any child dendrite branches
        const auto& childBranches = dendrite->getDendriteBranches();
        for (const auto& childBranch : childBranches) {
            updateDendriteBranch(txn, childBranch);
        }
    }
}

void updateDatabase(pqxx::connection& conn) {
    while (running) {
        std::unique_lock<std::mutex> lock(changedNeuronsMutex);
        cv.wait(lock, [] { return dbUpdateReady.load(); });

        if (!running && !dbUpdateReady.load()) {
            break;
        }

        // Copy and clear changed neurons and clusters
        std::unordered_set<std::shared_ptr<Neuron>> neuronsToUpdate = std::move(changedNeurons);
        changedNeurons.clear();

        std::unordered_set<std::shared_ptr<Cluster>> clustersToUpdate = std::move(changedClusters);
        changedClusters.clear();

        dbUpdateReady = false;
        lock.unlock();

        try {
            pqxx::work txn(conn);

            // Update clusters
            for (const auto& cluster : clustersToUpdate) {
                if (!cluster) continue;

                double x = cluster->getPosition()->x;
                double y = cluster->getPosition()->y;
                double z = cluster->getPosition()->z;
                double propagation_rate = cluster->getPropagationRate();
                double energy_level = cluster->getEnergyLevel();
                int cluster_id = cluster->getClusterId();

                txn.exec_params(
                        "UPDATE clusters SET x = $1, y = $2, z = $3, propagation_rate = $4, energy_level = $5 WHERE cluster_id = $6",
                        x, y, z, propagation_rate, energy_level, cluster_id
                );
            }

            // Update neurons and their components
            for (const auto& neuron : neuronsToUpdate) {
                if (!neuron) continue;

                // Update Neuron
                double neuron_x = neuron->getPosition()->x;
                double neuron_y = neuron->getPosition()->y;
                double neuron_z = neuron->getPosition()->z;
                double neuron_propagation_rate = neuron->getPropagationRate();
                double neuron_energy_level = neuron->getEnergyLevel();
                int neuron_id = neuron->getNeuronId();

                txn.exec_params(
                        "UPDATE neurons SET x = $1, y = $2, z = $3, propagation_rate = $4, energy_level = $5 WHERE neuron_id = $6",
                        neuron_x, neuron_y, neuron_z, neuron_propagation_rate, neuron_energy_level, neuron_id
                );

                // Update Soma
                auto soma = neuron->getSoma();
                if (soma) {
                    double soma_x = soma->getPosition()->x;
                    double soma_y = soma->getPosition()->y;
                    double soma_z = soma->getPosition()->z;
                    double soma_energy_level = soma->getEnergyLevel();
                    int soma_id = soma->getSomaId();

                    txn.exec_params(
                            "UPDATE somas SET x = $1, y = $2, z = $3, energy_level = $4 WHERE soma_id = $5",
                            soma_x, soma_y, soma_z, soma_energy_level, soma_id
                    );

                    // Update Axon Hillock
                    auto axonHillock = soma->getAxonHillock();
                    if (axonHillock) {
                        double axon_hillock_x = axonHillock->getPosition()->x;
                        double axon_hillock_y = axonHillock->getPosition()->y;
                        double axon_hillock_z = axonHillock->getPosition()->z;
                        double axon_hillock_energy_level = axonHillock->getEnergyLevel();
                        int axon_hillock_id = axonHillock->getAxonHillockId();

                        txn.exec_params(
                                "UPDATE axonhillocks SET x = $1, y = $2, z = $3, energy_level = $4 WHERE axon_hillock_id = $5",
                                axon_hillock_x, axon_hillock_y, axon_hillock_z, axon_hillock_energy_level, axon_hillock_id
                        );

                        // Update Axon and its components
                        auto axon = axonHillock->getAxon();
                        if (axon) {
                            updateAxon(txn, axon);
                        }
                    }

                    // Update Dendrite Branches
                    const auto& dendriteBranches = soma->getDendriteBranches();
                    for (const auto& dendriteBranch : dendriteBranches) {
                        updateDendriteBranch(txn, dendriteBranch);
                    }
                }
            }

            txn.commit();
        } catch (const std::exception& e) {
            std::cerr << "Database update error: " << e.what() << std::endl;
        }
    }
}

// Helper functions to update Axon and Dendrite components

