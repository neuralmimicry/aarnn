// database.cpp

/**
 * database.cpp
 *
 * Complete database integration with PostgreSQL (libpqxx)
 * - Schema initialization
 * - Prepared statements
 * - Pipelined updates
 * - Batch insertion with recursive helpers
 */

#include "database.h"
#include <pqxx/pqxx>
#include <iostream>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <vector>
#include <sstream>

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

// -----------------------------------------------------------------------------
// Globals (from main.cpp)
// -----------------------------------------------------------------------------
extern std::atomic<bool> running;
extern std::atomic<bool> dbUpdateReady;
extern std::mutex changedNeuronsMutex;
extern std::condition_variable cv;

// -----------------------------------------------------------------------------
// Forward declarations for insertion helpers
// -----------------------------------------------------------------------------
void insertAxonBranches(
        pqxx::transaction_base& txn,
        const std::shared_ptr<AxonBranch>& branch,
        int parentId,
        bool parentIsHillock
);
void insertDendriteBranches(
        pqxx::transaction_base& txn,
        const std::shared_ptr<DendriteBranch>& branch,
        int parentId,
        bool parentIsSoma
);

// -----------------------------------------------------------------------------
// 1) Schema initialization
// -----------------------------------------------------------------------------
void initialise_database(pqxx::connection& conn) {
    {
        pqxx::work txn{conn};
        txn.exec("DROP TABLE IF EXISTS dendriteboutons, dendrites, dendritebranches, "
                 "synapticgaps, axonboutons, axons, axonbranches, axonhillocks, "
                 "somas, neurons, clusters CASCADE;");
        txn.commit();
    }
    {
        pqxx::work txn{conn};
        txn.exec(
                "CREATE TABLE clusters (cluster_id SERIAL PRIMARY KEY, x REAL NOT NULL, y REAL NOT NULL, z REAL NOT NULL, propagation_rate REAL, cluster_type INTEGER, energy_level REAL NOT NULL, max_energy_level REAL NOT NULL);"
                "CREATE TABLE neurons  (neuron_id SERIAL PRIMARY KEY, cluster_id INTEGER, x REAL NOT NULL, y REAL NOT NULL, z REAL NOT NULL, propagation_rate REAL, neuron_type INTEGER, energy_level REAL NOT NULL, max_energy_level REAL NOT NULL);"
                "CREATE TABLE somas    (soma_id SERIAL PRIMARY KEY, neuron_id INTEGER, x REAL NOT NULL, y REAL NOT NULL, z REAL NOT NULL, energy_level REAL NOT NULL, max_energy_level REAL NOT NULL);"
                "CREATE TABLE axonhillocks (axon_hillock_id SERIAL PRIMARY KEY, soma_id INTEGER, x REAL NOT NULL, y REAL NOT NULL, z REAL NOT NULL, energy_level REAL NOT NULL, max_energy_level REAL NOT NULL);"
                "CREATE TABLE axonbranches (axon_branch_id SERIAL PRIMARY KEY, parent_axon_id INTEGER, parent_axon_branch_id INTEGER, x REAL NOT NULL, y REAL NOT NULL, z REAL NOT NULL, energy_level REAL NOT NULL, max_energy_level REAL NOT NULL);"
                "CREATE TABLE axons     (axon_id SERIAL PRIMARY KEY, axon_hillock_id INTEGER, axon_branch_id INTEGER, x REAL NOT NULL, y REAL NOT NULL, z REAL NOT NULL, energy_level REAL NOT NULL, max_energy_level REAL NOT NULL);"
                "CREATE TABLE axonboutons (axon_bouton_id SERIAL PRIMARY KEY, axon_id INTEGER, x REAL NOT NULL, y REAL NOT NULL, z REAL NOT NULL, energy_level REAL NOT NULL, max_energy_level REAL NOT NULL);"
                "CREATE TABLE synapticgaps (synaptic_gap_id SERIAL PRIMARY KEY, axon_bouton_id INTEGER, x REAL NOT NULL, y REAL NOT NULL, z REAL NOT NULL, energy_level REAL NOT NULL, max_energy_level REAL NOT NULL);"
                "CREATE TABLE dendritebranches (dendrite_branch_id SERIAL PRIMARY KEY, soma_id INTEGER, dendrite_id INTEGER, x REAL NOT NULL, y REAL NOT NULL, z REAL NOT NULL, energy_level REAL NOT NULL, max_energy_level REAL NOT NULL);"
                "CREATE TABLE dendrites (dendrite_id SERIAL PRIMARY KEY, dendrite_branch_id INTEGER, x REAL NOT NULL, y REAL NOT NULL, z REAL NOT NULL, energy_level REAL NOT NULL, max_energy_level REAL NOT NULL);"
                "CREATE TABLE dendriteboutons (dendrite_bouton_id SERIAL PRIMARY KEY, dendrite_id INTEGER UNIQUE, x REAL NOT NULL, y REAL NOT NULL, z REAL NOT NULL, energy_level REAL NOT NULL, max_energy_level REAL NOT NULL);"
        );
        txn.exec(
                "ALTER TABLE neurons        ADD FOREIGN KEY (cluster_id)       REFERENCES clusters(cluster_id);"
                "ALTER TABLE somas          ADD FOREIGN KEY (neuron_id)        REFERENCES neurons(neuron_id);"
                "ALTER TABLE axonhillocks   ADD FOREIGN KEY (soma_id)          REFERENCES somas(soma_id);"
                "ALTER TABLE axons          ADD FOREIGN KEY (axon_hillock_id)  REFERENCES axonhillocks(axon_hillock_id),"
                "                            ADD FOREIGN KEY (axon_branch_id)     REFERENCES axonbranches(axon_branch_id);"
                "ALTER TABLE axonbranches   ADD FOREIGN KEY (parent_axon_id)        REFERENCES axons(axon_id),"
                "                            ADD FOREIGN KEY (parent_axon_branch_id) REFERENCES axonbranches(axon_branch_id);"
                "ALTER TABLE axonboutons    ADD FOREIGN KEY (axon_id)          REFERENCES axons(axon_id);"
                "ALTER TABLE synapticgaps   ADD FOREIGN KEY (axon_bouton_id)   REFERENCES axonboutons(axon_bouton_id);"
                "ALTER TABLE dendritebranches ADD FOREIGN KEY (soma_id)        REFERENCES somas(soma_id),"
                "                             ADD FOREIGN KEY (dendrite_id)    REFERENCES dendrites(dendrite_id);"
                "ALTER TABLE dendrites       ADD FOREIGN KEY (dendrite_branch_id) REFERENCES dendritebranches(dendrite_branch_id);"
                "ALTER TABLE dendriteboutons ADD FOREIGN KEY (dendrite_id)     REFERENCES dendrites(dendrite_id);"
        );
        txn.commit();
        std::cout << "[DB INIT] Schema created.\n";
    }
}

// -----------------------------------------------------------------------------
// 2) Prepare all update statements once
// -----------------------------------------------------------------------------
void prepareUpdateStatements(pqxx::connection& conn) {
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
}

// -----------------------------------------------------------------------------
// 3) Helper: build EXECUTE call
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// 4) Pipelined updates (called from main)
// -----------------------------------------------------------------------------
void updateDatabase(pqxx::connection& conn,
                    const std::vector<std::shared_ptr<Cluster>>& clusters) {
    prepareUpdateStatements(conn);
    while (running) {
        std::unique_lock<std::mutex> lk(changedNeuronsMutex);
        cv.wait(lk, []{ return dbUpdateReady.load(); });
        if (!running && !dbUpdateReady.load()) break;
        dbUpdateReady = false;
        lk.unlock();

        try {
            pqxx::work txn{conn};
            pqxx::pipeline pipe{txn};
            for (auto const& c : clusters) {
                if (!c) continue;
                pipe.insert(make_execute(txn, "update_cluster",
                                         c->getPosition()->x,
                                         c->getPosition()->y,
                                         c->getPosition()->z,
                                         c->getPropagationRate(),
                                         c->getEnergyLevel(),
                                         c->getMaxEnergyLevel(),
                                         c->getClusterId()));
                // iterate neurons, somas, hillocks, axons, boutons, gaps
                for (auto const& n : c->getNeurons()) {
                    if (!n) continue;
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
                        pipe.insert(make_execute(txn, "update_soma",
                                                 s->getPosition()->x,
                                                 s->getPosition()->y,
                                                 s->getPosition()->z,
                                                 s->getEnergyLevel(),
                                                 s->getMaxEnergyLevel(),
                                                 s->getSomaId()));
                        auto ah = s->getAxonHillock();
                        if (ah) {
                            pipe.insert(make_execute(txn, "update_axonhillock",
                                                     ah->getPosition()->x,
                                                     ah->getPosition()->y,
                                                     ah->getPosition()->z,
                                                     ah->getEnergyLevel(),
                                                     ah->getMaxEnergyLevel(),
                                                     ah->getAxonHillockId()));
                            auto ax = ah->getAxon();
                            if (ax) {
                                pipe.insert(make_execute(txn, "update_axon",
                                                         ax->getPosition()->x,
                                                         ax->getPosition()->y,
                                                         ax->getPosition()->z,
                                                         ax->getEnergyLevel(),
                                                         ax->getMaxEnergyLevel(),
                                                         ax->getAxonId()));
                                auto btn = ax->getAxonBouton();
                                if (btn) {
                                    pipe.insert(make_execute(txn, "update_axonbouton",
                                                             btn->getPosition()->x,
                                                             btn->getPosition()->y,
                                                             btn->getPosition()->z,
                                                             btn->getEnergyLevel(),
                                                             btn->getMaxEnergyLevel(),
                                                             btn->getAxonBoutonId()));
                                    auto gap = btn->getSynapticGap();
                                    if (gap) {
                                        pipe.insert(make_execute(txn, "update_synapticgap",
                                                                 gap->getPosition()->x,
                                                                 gap->getPosition()->y,
                                                                 gap->getPosition()->z,
                                                                 gap->getEnergyLevel(),
                                                                 gap->getMaxEnergyLevel(),
                                                                 gap->getSynapticGapId()));
                                    }
                                }
                                // axon branches recursion
                                for (auto const& br : ax->getAxonBranches())
                                    insertAxonBranches(txn, br, ax->getAxonId(), true);
                            }
                        }
                        // dendrite branches recursion
                        for (auto const& br : s->getDendriteBranches())
                            insertDendriteBranches(txn, br, s->getSomaId(), true);
                    }
                }
            }
            pipe.complete();
            txn.commit();
        } catch (const std::exception& e) {
            std::cerr << "[DB UPDATE] " << e.what() << "\n";
        }
    }
}

// -----------------------------------------------------------------------------
// 5) Batch insert clusters & sub-components
// -----------------------------------------------------------------------------
void batch_insert_clusters(pqxx::transaction_base& txn,
                           const std::vector<std::shared_ptr<Cluster>>& clusters) {
    for (auto const& c : clusters) {
        if (!c) continue;
        // cluster
        auto cr = txn.exec_params(
                "INSERT INTO clusters (x,y,z,propagation_rate,cluster_type,energy_level,max_energy_level) "
                "VALUES ($1,$2,$3,$4,$5,$6,$7) RETURNING cluster_id",
                c->getPosition()->x, c->getPosition()->y, c->getPosition()->z,
                c->getPropagationRate(), c->getClusterType(),
                c->getEnergyLevel(), c->getMaxEnergyLevel());
        int cid = cr[0][0].as<int>(); c->setClusterId(cid);
        // neurons, somas, etc...
        for (auto const& n : c->getNeurons()) {
            if (!n||!n->getSoma()) continue;
            auto nr = txn.exec_params(
                    "INSERT INTO neurons (cluster_id,x,y,z,propagation_rate,neuron_type,energy_level,max_energy_level) "
                    "VALUES ($1,$2,$3,$4,$5,$6,$7,$8) RETURNING neuron_id",
                    cid, n->getPosition()->x, n->getPosition()->y, n->getPosition()->z,
                    n->getPropagationRate(), n->getNeuronType(), n->getEnergyLevel(), n->getMaxEnergyLevel());
            int nid = nr[0][0].as<int>(); n->setNeuronId(nid);
            auto sr = txn.exec_params(
                    "INSERT INTO somas (neuron_id,x,y,z,energy_level,max_energy_level) "
                    "VALUES ($1,$2,$3,$4,$5,$6) RETURNING soma_id",
                    nid, n->getSoma()->getPosition()->x, n->getSoma()->getPosition()->y,
                    n->getSoma()->getPosition()->z, n->getSoma()->getEnergyLevel(), n->getSoma()->getMaxEnergyLevel());
            int sid = sr[0][0].as<int>(); n->getSoma()->setSomaId(sid);
            // hillock + axon + bouton + gap + branches
            auto ah = n->getSoma()->getAxonHillock();
            if (ah) {
                auto ahr = txn.exec_params(
                        "INSERT INTO axonhillocks (soma_id,x,y,z,energy_level,max_energy_level) "
                        "VALUES ($1,$2,$3,$4,$5,$6) RETURNING axon_hillock_id", sid,
                        ah->getPosition()->x, ah->getPosition()->y, ah->getPosition()->z,
                        ah->getEnergyLevel(), ah->getMaxEnergyLevel());
                int ahid = ahr[0][0].as<int>(); ah->setAxonHillockId(ahid);
                auto ax = ah->getAxon();
                if (ax) {
                    auto axr = txn.exec_params(
                            "INSERT INTO axons (axon_hillock_id,x,y,z,energy_level,max_energy_level) "
                            "VALUES ($1,$2,$3,$4,$5,$6) RETURNING axon_id", ahid,
                            ax->getPosition()->x, ax->getPosition()->y, ax->getPosition()->z,
                            ax->getEnergyLevel(), ax->getMaxEnergyLevel());
                    int axid = axr[0][0].as<int>(); ax->setAxonId(axid);
                    auto btn = ax->getAxonBouton();
                    if (btn) {
                        auto btr = txn.exec_params(
                                "INSERT INTO axonboutons (axon_id,x,y,z,energy_level,max_energy_level) "
                                "VALUES ($1,$2,$3,$4,$5,$6) RETURNING axon_bouton_id", axid,
                                btn->getPosition()->x, btn->getPosition()->y, btn->getPosition()->z,
                                btn->getEnergyLevel(), btn->getMaxEnergyLevel());
                        btn->setAxonBoutonId(btr[0][0].as<int>());
                        auto gap = btn->getSynapticGap();
                        if (gap) {
                            auto sgr = txn.exec_params(
                                    "INSERT INTO synapticgaps (axon_bouton_id,x,y,z,energy_level,max_energy_level) "
                                    "VALUES ($1,$2,$3,$4,$5,$6) RETURNING synaptic_gap_id", btn->getAxonBoutonId(),
                                    gap->getPosition()->x, gap->getPosition()->y, gap->getPosition()->z,
                                    gap->getEnergyLevel(), gap->getMaxEnergyLevel());
                            gap->setSynapticGapId(sgr[0][0].as<int>());
                        }
                    }
                    for (auto const& br : ax->getAxonBranches())
                        insertAxonBranches(txn, br, ahid, true);
                }
            }
            for (auto const& db : n->getSoma()->getDendriteBranches())
                insertDendriteBranches(txn, db, sid, true);
        }
    }
}

// -----------------------------------------------------------------------------
// Implementation: recursive AxonBranch insertion
// -----------------------------------------------------------------------------
void insertAxonBranches(
        pqxx::transaction_base& txn,
        const std::shared_ptr<AxonBranch>& branch,
        int parentId,
        bool parentIsHillock
) {
    if (!branch) return;
    int bid;
    const char* sql = parentIsHillock
                      ? "INSERT INTO axonbranches (parent_axon_id,x,y,z,energy_level,max_energy_level) VALUES ($1,$2,$3,$4,$5,$6) RETURNING axon_branch_id"
                      : "INSERT INTO axonbranches (parent_axon_branch_id,x,y,z,energy_level,max_energy_level) VALUES ($1,$2,$3,$4,$5,$6) RETURNING axon_branch_id";
    auto r = txn.exec_params(sql,
                             parentId,
                             branch->getPosition()->x,
                             branch->getPosition()->y,
                             branch->getPosition()->z,
                             branch->getEnergyLevel(),
                             branch->getMaxEnergyLevel());
    bid = r[0][0].as<int>();
    branch->setAxonBranchId(bid);
    // children: from branch.getAxons(), then recuse on each child's getAxonBranches()
    for (auto const& ax : branch->getAxons()) {
        for (auto const& sub : ax->getAxonBranches()) {
            insertAxonBranches(txn, sub, bid, false);
        }
    }
}

// -----------------------------------------------------------------------------
// Implementation: recursive DendriteBranch insertion
// -----------------------------------------------------------------------------
void insertDendriteBranches(
        pqxx::transaction_base& txn,
        const std::shared_ptr<DendriteBranch>& branch,
        int parentId,
        bool parentIsSoma
) {
    if (!branch) return;
    int bid;
    const char* sql = parentIsSoma
                      ? "INSERT INTO dendritebranches (soma_id,x,y,z,energy_level,max_energy_level) VALUES ($1,$2,$3,$4,$5,$6) RETURNING dendrite_branch_id"
                      : "INSERT INTO dendritebranches (dendrite_id,x,y,z,energy_level,max_energy_level) VALUES ($1,$2,$3,$4,$5,$6) RETURNING dendrite_branch_id";
    auto r = txn.exec_params(sql,
                             parentId,
                             branch->getPosition()->x,
                             branch->getPosition()->y,
                             branch->getPosition()->z,
                             branch->getEnergyLevel(),
                             branch->getMaxEnergyLevel());
    bid = r[0][0].as<int>();
    branch->setDendriteBranchId(bid);
    for (auto const& d : branch->getDendrites()) {
        auto dr = txn.exec_params(
                "INSERT INTO dendrites (dendrite_branch_id,x,y,z,energy_level,max_energy_level) VALUES ($1,$2,$3,$4,$5,$6) RETURNING dendrite_id",
                bid,
                d->getPosition()->x,
                d->getPosition()->y,
                d->getPosition()->z,
                d->getEnergyLevel(),
                d->getMaxEnergyLevel());
        int did = dr[0][0].as<int>();
        d->setDendriteId(did);
        auto btn = d->getDendriteBouton();
        if (btn) {
            auto br2 = txn.exec_params(
                    "INSERT INTO dendriteboutons (dendrite_id,x,y,z,energy_level,max_energy_level) VALUES ($1,$2,$3,$4,$5,$6) RETURNING dendrite_bouton_id",
                    did,
                    btn->getPosition()->x,
                    btn->getPosition()->y,
                    btn->getPosition()->z,
                    btn->getEnergyLevel(),
                    btn->getMaxEnergyLevel());
            btn->setDendriteBoutonId(br2[0][0].as<int>());
        }
        for (auto const& sub : d->getDendriteBranches())
            insertDendriteBranches(txn, sub, did, false);
    }
}
