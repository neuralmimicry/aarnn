//
// Created by pbisaacs on 23/06/24.
//
#include "database.h"

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
        pqxx::result result = txn.exec("SELECT EXISTS ("
                                       "SELECT FROM pg_tables WHERE schemaname = 'public' AND tablename = 'clusters'"
                                       ");");

        if (!result.empty() && !result[0][0].as<bool>()) {
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
        } else {
            std::cout << "Table 'neurons' already exists." << std::endl;
        }
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
        std::cout << "Inserted Axon Bouton ID: " << axon_bouton_id << std::endl;

        // Insert Synaptic Gap
        txn.exec_params(
                "INSERT INTO synapticgaps (axon_bouton_id, x, y, z, energy_level) VALUES ($1, $2, $3, $4, $5)",
                axon_bouton_id,
                axon->getAxonBouton()->getSynapticGap()->getPosition()->x,
                axon->getAxonBouton()->getSynapticGap()->getPosition()->y,
                axon->getAxonBouton()->getSynapticGap()->getPosition()->z,
                axon->getAxonBouton()->getSynapticGap()->getEnergyLevel()
        );
        std::cout << "Inserted Synaptic Gap for Axon Bouton ID: " << axon_bouton_id << std::endl;

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
            std::cout << "Inserted Cluster ID: " << cluster_id << std::endl;

            if (cluster->getNeurons().empty()) {
                std::cerr << "Cluster " << outercount << " has no neurons. Skipping neuron insertion." << std::endl;
                continue;
            }

            int count = 0;
            for (const auto &neuron: cluster->getNeurons()) {
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
                    if (!neuron->getSoma()->getAxonHillock()) {
                        std::cerr << "Neuron " << count << " in cluster " << outercount << " has a null AxonHillock. Skipping insertion." << std::endl;
                        continue;
                    }
                    if (!neuron->getSoma()->getAxonHillock()->getAxon()) {
                        std::cerr << "Neuron " << count << " in cluster " << outercount << " has a null Axon. Skipping insertion." << std::endl;
                        continue;
                    }
                    if (!neuron->getSoma()->getAxonHillock()->getAxon()->getAxonBouton()) {
                        std::cerr << "Neuron " << count << " in cluster " << outercount << " has a null AxonBouton. Skipping insertion." << std::endl;
                        continue;
                    }
                    if (!neuron->getSoma()->getAxonHillock()->getAxon()->getAxonBouton()->getSynapticGap()) {
                        std::cerr << "Neuron " << count << " in cluster " << outercount << " has a null SynapticGap. Skipping insertion." << std::endl;
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
                    std::cout << "Inserted Soma ID: " << soma_id << std::endl;

                    // Insert Axon Hillock and get axon_hillock_id
                    pqxx::result axon_hillock_result = txn.exec_params(
                            "INSERT INTO axonhillocks (soma_id, x, y, z, energy_level) VALUES ($1, $2, $3, $4, $5) RETURNING axon_hillock_id",
                            soma_id,
                            neuron->getSoma()->getAxonHillock()->getPosition()->x,
                            neuron->getSoma()->getAxonHillock()->getPosition()->y,
                            neuron->getSoma()->getAxonHillock()->getPosition()->z,
                            neuron->getSoma()->getAxonHillock()->getEnergyLevel()
                    );
                    int axon_hillock_id = axon_hillock_result[0][0].as<int>();
                    std::cout << "Inserted Axon Hillock ID: " << axon_hillock_id << std::endl;

                    // Insert Axon and get axon_id
                    pqxx::result axon_result = txn.exec_params(
                            "INSERT INTO axons (axon_hillock_id, x, y, z, energy_level) VALUES ($1, $2, $3, $4, $5) RETURNING axon_id",
                            axon_hillock_id,
                            neuron->getSoma()->getAxonHillock()->getAxon()->getPosition()->x,
                            neuron->getSoma()->getAxonHillock()->getAxon()->getPosition()->y,
                            neuron->getSoma()->getAxonHillock()->getAxon()->getPosition()->z,
                            neuron->getSoma()->getAxonHillock()->getAxon()->getEnergyLevel()
                    );
                    int axon_id = axon_result[0][0].as<int>();
                    std::cout << "Inserted Axon ID: " << axon_id << std::endl;

                    // Insert Axon Bouton and get axon_bouton_id
                    pqxx::result axon_bouton_result = txn.exec_params(
                            "INSERT INTO axonboutons (axon_id, x, y, z, energy_level) VALUES ($1, $2, $3, $4, $5) RETURNING axon_bouton_id",
                            axon_id,
                            neuron->getSoma()->getAxonHillock()->getAxon()->getAxonBouton()->getPosition()->x,
                            neuron->getSoma()->getAxonHillock()->getAxon()->getAxonBouton()->getPosition()->y,
                            neuron->getSoma()->getAxonHillock()->getAxon()->getAxonBouton()->getPosition()->z,
                            neuron->getSoma()->getAxonHillock()->getAxon()->getAxonBouton()->getEnergyLevel()
                    );
                    int axon_bouton_id = axon_bouton_result[0][0].as<int>();
                    std::cout << "Inserted Axon Bouton ID: " << axon_bouton_id << std::endl;

                    // Insert Synaptic Gap
                    txn.exec_params(
                            "INSERT INTO synapticgaps (axon_bouton_id, x, y, z, energy_level) VALUES ($1, $2, $3, $4, $5)",
                            axon_bouton_id,
                            neuron->getSoma()->getAxonHillock()->getAxon()->getAxonBouton()->getSynapticGap()->getPosition()->x,
                            neuron->getSoma()->getAxonHillock()->getAxon()->getAxonBouton()->getSynapticGap()->getPosition()->y,
                            neuron->getSoma()->getAxonHillock()->getAxon()->getAxonBouton()->getSynapticGap()->getPosition()->z,
                            neuron->getSoma()->getAxonHillock()->getAxon()->getAxonBouton()->getSynapticGap()->getEnergyLevel()
                    );
                    std::cout << "Inserted Synaptic Gap for Axon Bouton ID: " << axon_bouton_id << std::endl;

                    // Insert Axon Branches recursively starting from the Axon Hillock's Axon
                    for (const auto &axonBranch: neuron->getSoma()->getAxonHillock()->getAxon()->getAxonBranches()) {
                        insertAxonBranches(txn, axonBranch, axon_id, false);
                    }

                    // Insert Dendrite Branches recursively
                    for (const auto &dendriteBranch: neuron->getSoma()->getDendriteBranches()) {
                        insertDendriteBranches(txn, dendriteBranch, soma_id, true);
                    }
                }
                catch (const std::exception &e) {
                    std::cerr << "Error inserting neuron " << count << " in cluster " << outercount << ": " << e.what() << std::endl;
                }
            }
        }
        catch (const std::exception &e) {
            std::cerr << "Error inserting cluster " << outercount << ": " << e.what() << std::endl;
        }
    }
}

void updateDatabase(pqxx::connection& conn) {
    while (running) {
        std::unique_lock<std::mutex> lock(changedNeuronsMutex);
        cv.wait_for(lock, std::chrono::milliseconds(250), [] { return dbUpdateReady.load(); });

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
                txn.exec_params(
                        "UPDATE clusters SET x = $1, y = $2, z = $3, propagation_rate = $4, energy_level = $5 WHERE cluster_id = $6",
                        cluster->getPosition()->x, cluster->getPosition()->y, cluster->getPosition()->z,
                        cluster->getPropagationRate(), cluster->getEnergyLevel(), cluster->getClusterId()
                );
            }

            // Update neurons
            for (const auto& neuron : neuronsToUpdate) {
                txn.exec_params(
                        "UPDATE neurons SET x = $1, y = $2, z = $3, propagation_rate = $4, energy_level = $5 WHERE neuron_id = $6",
                        neuron->getPosition()->x, neuron->getPosition()->y, neuron->getPosition()->z,
                        neuron->getPropagationRate(), neuron->getEnergyLevel(), neuron->getNeuronId()
                );
            }

            txn.commit();
        } catch (const std::exception& e) {
            std::cerr << "Database update error: " << e.what() << std::endl;
        }
    }
}
