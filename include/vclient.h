//
// Created by pbisaacs on 15/06/24.
//

#ifndef AARNN_VCLIENT_H
#define AARNN_VCLIENT_H
#include <string>

// Function to handle the write callback for CURL
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);

// Function to retrieve Postgres credentials from Vault
bool getPostgresCredentials(const std::string& vault_addr, const std::string& vault_token,
                            const std::string& secret_path, std::string& username,
                            std::string& password, std::string& database, std::string& database_host,
                            std::string& database_port);

// Function to initialise the database connection
bool initialiseDatabaseConnection(std::string& connection_string);

#endif //AARNN_VCLIENT_H
