#include "vclient.h"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <sstream>
#include <stdexcept>

using json = nlohmann::json;

// Function to handle the write callback for CURL
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Function to retrieve Postgres credentials from Vault
bool getPostgresCredentials(const std::string& vault_api_addr, const std::string& vault_token,
                            const std::string& secret_path, std::string& username,
                            std::string& password, std::string& database, std::string& database_host, std::string& database_port) {
    CURL* curl;
    CURLcode res;
    curl = curl_easy_init();
    if(curl) {
        std::string url = vault_api_addr + "/v1/" + secret_path;
        std::string response_string;
        std::cout << "Vault API URL: " << url << std::endl;

        // Set the URL and headers
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, ("X-Vault-Token: " + vault_token).c_str());
        std::cout << "Vault Token: " << vault_token << std::endl;
        std::cout << "Headers: " << headers << std::endl;
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);

        // Perform the request
        res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
            curl_easy_cleanup(curl);
            return false;
        }

        // Cleanup
        curl_easy_cleanup(curl);

        // Parse the JSON response
        json jsonData;
        try {
            jsonData = json::parse(response_string);
        }
        catch (const json::parse_error& e) {
            std::cerr << "Failed to parse JSON: " << e.what() << "\n";
            return false;
        }

        // Vault wraps your secret under data.data
        // like { "data": { "data": { "POSTGRES_USERNAME": "...", … } } }
        if (!jsonData.contains("data") || !jsonData["data"].contains("data") || !jsonData["data"]["data"].is_object()) {
            std::cerr << "Unexpected JSON structure in Vault response\n";
            return false;
        }
        json creds;
        auto outer = jsonData.find("data");
        if (outer != jsonData.end()) {
            if (outer->contains("data") && (*outer)["data"].is_object()) {
                creds = (*outer)["data"]; // KV v2
            } else {
                creds = *outer; // KV v1
            }
        } else {
            std::cerr << "Invalid Vault response: no 'data' field\n";
            return false;
        }

        try {
            username      = creds.at("POSTGRES_USERNAME").get<std::string>();
            password      = creds.at("POSTGRES_PASSWORD").get<std::string>();
            database      = creds.at("POSTGRES_DB").       get<std::string>();
            database_host = creds.at("POSTGRES_HOST").     get<std::string>();
            database_port = creds.at("POSTGRES_PORT").     get<std::string>();
        }
        catch (const json::out_of_range& e) {
            std::cerr << "Missing key in Vault JSON: " << e.what() << "\n";
            return false;
        }
        catch (const json::type_error& e) {
            std::cerr << "Type error in Vault JSON: " << e.what() << "\n";
            return false;
        }

        return true;
    }

    return false;
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
    return true;
}
