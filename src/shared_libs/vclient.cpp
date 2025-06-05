#include "vclient.h"
#include <curl/curl.h>
#include <boost/json.hpp>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace json = boost::json;

// Function to handle the write callback for CURL
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Function to retrieve Postgres credentials from Vault
bool getPostgresCredentials(const std::string& vault_api_addr,
                            const std::string& vault_token,
                            const std::string& secret_path,
                            std::string& username,
                            std::string& password,
                            std::string& database,
                            std::string& database_host,
                            std::string& database_port)
{
    CURL* curl = curl_easy_init();
    if (!curl) {
        return false;
    }

    std::string url = vault_api_addr + "/v1/" + secret_path;
    std::string response_string;
    std::cout << "Vault API URL: " << url << std::endl;

    // Set up CURL
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, ("X-Vault-Token: " + vault_token).c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        curl_easy_cleanup(curl);
        return false;
    }

    curl_easy_cleanup(curl);

    // Parse the JSON response with Boost.JSON
    json::value jsonData;
    try {
        jsonData = json::parse(response_string);
    }
    catch (const json::system_error& e) {
        std::cerr << "Failed to parse JSON: " << e.what() << "\n";
        return false;
    }

    if (!jsonData.is_object()) {
        std::cerr << "Unexpected JSON structure: top‐level is not an object\n";
        return false;
    }
    json::object& topObj = jsonData.as_object();

    // Drill down into data.data for KV v2
    if (!topObj.contains("data") || !topObj["data"].is_object()) {
        std::cerr << "Unexpected JSON structure in Vault response: missing top‐level \"data\"\n";
        return false;
    }
    json::object& dataObj = topObj["data"].as_object();

    if (!dataObj.contains("data") || !dataObj["data"].is_object()) {
        std::cerr << "Unexpected JSON structure in Vault response: missing nested \"data\"\n";
        return false;
    }
    json::object& credsObj = dataObj["data"].as_object();

    try {
        username      = std::string(credsObj.at("POSTGRES_USERNAME").as_string());
        password      = std::string(credsObj.at("POSTGRES_PASSWORD").as_string());
        database      = std::string(credsObj.at("POSTGRES_DB").as_string());
        database_host = std::string(credsObj.at("POSTGRES_HOST").as_string());
        database_port = std::string(credsObj.at("POSTGRES_PORT").as_string());
    }
    catch (const std::out_of_range& e) {
        std::cerr << "Missing key in Vault JSON: " << e.what() << "\n";
        return false;
    }
    catch (const json::system_error& e) {
        std::cerr << "Type error in Vault JSON: " << e.what() << "\n";
        return false;
    }

    return true;
}

// Function to initialise the database connection
bool initialiseDatabaseConnection(std::string& connection_string) {
    const char* vault_addr_env = std::getenv("VAULT_ADDR");
    const char* vault_api_addr_env = std::getenv("VAULT_API_ADDR");
    const char* vault_token_env = std::getenv("VAULT_TOKEN");

    if (!vault_api_addr_env || !vault_token_env) {
        std::cerr << "Environment variables VAULT_API_ADDR and VAULT_TOKEN must be set." << std::endl;
        return false;
    }

    std::string vault_api_addr = vault_api_addr_env;
    std::string vault_token = vault_token_env;
    std::cout << "Vault API Address: " << vault_api_addr << std::endl;
    std::cout << "Vault Token: " << vault_token << std::endl;
    std::string secret_path = "secret/data/postgres"; // Adjust this path based on your Vault setup
    std::string username, password, database, database_host, database_port;

    if (getPostgresCredentials(vault_api_addr, vault_token, secret_path, username, password, database, database_host, database_port)) {
        std::cout << "Postgres Username: " << username << std::endl;
        std::cout << "Postgres Password: " << password << std::endl;
        std::cout << "Postgres Database: " << database << std::endl;
        std::cout << "Postgres Database Host: " << database_host << std::endl;
        std::cout << "Postgres Database Port: " << database_port << std::endl;
    } else {
        std::cerr << "Failed to get Postgres credentials from Vault" << std::endl;
        return false;
    }

    if (username.empty()) {
        username = std::getenv("POSTGRES_USERNAME");
    }

    if (password.empty()) {
        password = std::getenv("POSTGRES_PASSWORD");
    }

    if (database.empty()) {
        database = std::getenv("POSTGRES_DB");
    }

    if (database_host.empty()) {
        database_host = std::getenv("POSTGRES_HOST");
    }

    if (database_port.empty()) {
        database_port = std::getenv("POSTGRES_PORT");
    }

    if (username.empty() || password.empty() || database.empty() || database_host.empty() || database_port.empty()) {
        std::cerr << "Postgres credentials not found in Vault nor environment variables" << std::endl;
        return false;
    }

    // Initialise database connection using the retrieved credentials
    connection_string = "dbname=" + database + " user=" + username + " password=" + password + " host=" + database_host + " port=" + database_port;
    std::cout << "Database connection string: " << connection_string << std::endl;
    return true;
}
