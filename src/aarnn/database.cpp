//
// Created by pbisaacs on 23/06/24.
//
#include "database.h"

void initialise_database(pqxx::connection& conn) {
    pqxx::work txn(conn);

    txn.exec("DROP TABLE IF EXISTS neurons, somas, axonhillocks, axons, axons_hillock, axonboutons, synapticgaps, dendritebranches_soma, dendritebranches, dendrites, dendriteboutons, axonbranches CASCADE; COMMIT;");

    pqxx::result result = txn.exec("SELECT EXISTS ("
                                   "SELECT FROM pg_tables WHERE schemaname = 'public' AND tablename = 'neurons'"
                                   ");");

    if (!result.empty() && !result[0][0].as<bool>()) {
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

void insertDendriteBranches(pqxx::transaction_base& txn, const std::shared_ptr<DendriteBranch>& dendriteBranch, int& dendrite_branch_id, int& dendrite_id, int& dendrite_bouton_id, int& soma_id) {
    if (!dendriteBranch) {
        std::cerr << "dendriteBranch is null!" << std::endl;
        return;
    }
    std::string query;

    if (dendriteBranch->getParentSoma()) {
        query = "INSERT INTO dendritebranches_soma (dendrite_branch_id, soma_id, x, y, z) VALUES (" +
                std::to_string(dendrite_branch_id) + ", " + std::to_string(soma_id) + ", " +
                std::to_string(dendriteBranch->getPosition()->x) + ", " +
                std::to_string(dendriteBranch->getPosition()->y) + ", " +
                std::to_string(dendriteBranch->getPosition()->z) + ")";
        txn.exec(query);
    } else {
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

        dendrite_id++;
        dendrite_bouton_id++;

        for (auto& innerDendriteBranch : dendrite->getDendriteBranches()) {
            dendrite_branch_id++;
            insertDendriteBranches(txn, innerDendriteBranch, dendrite_branch_id, dendrite_id, dendrite_bouton_id, soma_id);
        }
    }
    dendrite_branch_id++;
}

void insertAxonBranches(pqxx::transaction_base& txn, const std::shared_ptr<AxonBranch>& axonBranch, int& axon_branch_id, int& axon_id, int& axon_bouton_id, int& synaptic_gap_id, int& axon_hillock_id) {
    if (!axonBranch) {
        std::cerr << "axonBranch is null!" << std::endl;
        return;
    }
    std::string query;

    query = "INSERT INTO axonbranches (axon_branch_id, axon_id, x, y, z) VALUES (" +
            std::to_string(axon_branch_id) + ", " + std::to_string(axon_id) + ", " +
            std::to_string(axonBranch->getPosition()->x) + ", " +
            std::to_string(axonBranch->getPosition()->y) + ", " +
            std::to_string(axonBranch->getPosition()->z) + ")";
    txn.exec(query);

    for (auto& axon : axonBranch->getAxons()) {
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

        axon_bouton_id++;
        synaptic_gap_id++;

        for (auto& innerAxonBranch : axon->getAxonBranches()) {
            axon_branch_id++;
            insertAxonBranches(txn, innerAxonBranch, axon_branch_id, axon_id, axon_bouton_id, synaptic_gap_id, axon_hillock_id);
        }
    }
}

void batch_insert_neurons(pqxx::transaction_base& txn, const std::vector<std::shared_ptr<Neuron>>& neurons) {
    std::ostringstream neuron_insert;
    std::ostringstream soma_insert;
    std::ostringstream axon_hillock_insert;
    std::ostringstream axon_insert;
    std::ostringstream axon_bouton_insert;
    std::ostringstream synaptic_gap_insert;

    neuron_insert << "INSERT INTO neurons (neuron_id, x, y, z) VALUES ";
    soma_insert << "INSERT INTO somas (soma_id, neuron_id, x, y, z) VALUES ";
    axon_hillock_insert << "INSERT INTO axonhillocks (axon_hillock_id, soma_id, x, y, z) VALUES ";
    axon_insert << "INSERT INTO axons (axon_id, axon_hillock_id, x, y, z) VALUES ";
    axon_bouton_insert << "INSERT INTO axonboutons (axon_bouton_id, axon_id, x, y, z) VALUES ";
    synaptic_gap_insert << "INSERT INTO synapticgaps (synaptic_gap_id, axon_bouton_id, x, y, z) VALUES ";

    bool first = true;

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
        if (!first) {
            neuron_insert << ", ";
            soma_insert << ", ";
            axon_hillock_insert << ", ";
            axon_insert << ", ";
            axon_bouton_insert << ", ";
            synaptic_gap_insert << ", ";
        } else {
            first = false;
        }

        neuron_insert << "(" << neuron_id << ", " << neuron->getPosition()->x << ", " << neuron->getPosition()->y << ", " << neuron->getPosition()->z << ")";
        soma_insert << "(" << soma_id << ", " << neuron_id << ", " << neuron->getSoma()->getPosition()->x << ", " << neuron->getSoma()->getPosition()->y << ", " << neuron->getSoma()->getPosition()->z << ")";
        axon_hillock_insert << "(" << axon_hillock_id << ", " << soma_id << ", " << neuron->getSoma()->getAxonHillock()->getPosition()->x << ", " << neuron->getSoma()->getAxonHillock()->getPosition()->y << ", " << neuron->getSoma()->getAxonHillock()->getPosition()->z << ")";
        axon_insert << "(" << axon_id << ", " << axon_hillock_id << ", " << neuron->getSoma()->getAxonHillock()->getAxon()->getPosition()->x << ", " << neuron->getSoma()->getAxonHillock()->getAxon()->getPosition()->y << ", " << neuron->getSoma()->getAxonHillock()->getAxon()->getPosition()->z << ")";
        axon_bouton_insert << "(" << axon_bouton_id << ", " << axon_id << ", " << neuron->getSoma()->getAxonHillock()->getAxon()->getAxonBouton()->getPosition()->x << ", " << neuron->getSoma()->getAxonHillock()->getAxon()->getAxonBouton()->getPosition()->y << ", " << neuron->getSoma()->getAxonHillock()->getAxon()->getAxonBouton()->getPosition()->z << ")";
        synaptic_gap_insert << "(" << synaptic_gap_id << ", " << axon_bouton_id << ", " << neuron->getSoma()->getAxonHillock()->getAxon()->getAxonBouton()->getSynapticGap()->getPosition()->x << ", " << neuron->getSoma()->getAxonHillock()->getAxon()->getAxonBouton()->getSynapticGap()->getPosition()->y << ", " << neuron->getSoma()->getAxonHillock()->getAxon()->getAxonBouton()->getSynapticGap()->getPosition()->z << ")";

        axon_bouton_id++;
        synaptic_gap_id++;

        if (neuron &&
            neuron->getSoma() &&
            neuron->getSoma()->getAxonHillock() &&
            neuron->getSoma()->getAxonHillock()->getAxon() &&
            !neuron->getSoma()->getAxonHillock()->getAxon()->getAxonBranches().empty() &&
            neuron->getSoma()->getAxonHillock()->getAxon()->getAxonBranches()[0]) {

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

    txn.exec(neuron_insert.str());
    txn.exec(soma_insert.str());
    txn.exec(axon_hillock_insert.str());
    txn.exec(axon_insert.str());
    txn.exec(axon_bouton_insert.str());
    txn.exec(synaptic_gap_insert.str());
}

void updateDatabase(pqxx::connection& conn) {
    while (running) {
        std::unique_lock<std::mutex> lock(changedNeuronsMutex);
        cv.wait_for(lock, std::chrono::milliseconds(250), [] { return dbUpdateReady.load(); });

        if (!running && !dbUpdateReady.load()) {
            break;
        }

        std::unordered_set<std::shared_ptr<Neuron>> neuronsToUpdate = std::move(changedNeurons);
        changedNeurons.clear();
        dbUpdateReady = false;
        lock.unlock();

        try {
            pqxx::work txn(conn);

            for (const auto& neuron : neuronsToUpdate) {
                txn.exec_params(
                        "UPDATE neurons SET x = $1, y = $2, z = $3, propagation_rate = $4 WHERE neuron_id = $5",
                        neuron->getPosition()->x, neuron->getPosition()->y, neuron->getPosition()->z,
                        neuron->getSoma()->getPropagationRate(), neuron->getNeuronId()
                );
            }

            txn.commit();
        } catch (const std::exception& e) {
            std::cerr << "Database update error: " << e.what() << std::endl;
        }
    }
}
