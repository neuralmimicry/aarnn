//
// Created by pbisaacs on 23/06/24.
//

#ifndef AARNN_DATABASE_H
#define AARNN_DATABASE_H

#include <pqxx/pqxx>
#include <memory>
#include <vector>
#include <iostream>
#include <sstream>
#include "DendriteBranch.h"
#include "AxonBranch.h"
#include <condition_variable>
#include <unordered_set>
#include "globals.h"

void initialise_database(pqxx::connection& conn);

void batch_insert_clusters(pqxx::transaction_base& txn, const std::vector<std::shared_ptr<Cluster>>& clusters);

void updateDatabase(pqxx::connection& conn, const std::vector<std::shared_ptr<Cluster>>& clusters);

#endif //AARNN_DATABASE_H
