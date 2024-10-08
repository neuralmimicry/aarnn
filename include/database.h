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

void batch_insert_neurons(pqxx::transaction_base& txn, const std::vector<std::shared_ptr<Neuron>>& neurons);

void updateDatabase(pqxx::connection& conn);

#endif //AARNN_DATABASE_H
