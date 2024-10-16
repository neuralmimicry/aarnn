#include "vclient.h"
#include <curl/curl.h>
#include <json/json.h> // Include a JSON library, such as jsoncpp
#include <iostream>
#include <sstream>
#include <stdexcept>

// Function to handle the write callback for CURL
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Function to retrieve Postgres credentials from Vault
bool getPostgresCredentials(const std::string& vault_addr, const std::string& vault_token,
                            const std::string& secret_path, std::string& username,
                            std::string& password, std::string& database, std::string& database_host, std::string& database_port) {
    CURL* curl;
    CURLcode res;
    curl = curl_easy_init();
    if(curl) {
        std::string url = vault_addr + "/v1/" + secret_path;
        std::string response_string;
        std::cout << "Vault URL: " << url << std::endl;

        // Set the URL and headers
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, ("X-Vault-Token: " + vault_token).c_str());
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
        Json::Value jsonData;
        Json::CharReaderBuilder readerBuilder;
        std::string errs;
        std::istringstream iss(response_string);
        if (!Json::parseFromStream(readerBuilder, iss, &jsonData, &errs)) {
            std::cerr << "Failed to parse JSON: " << errs << std::endl;
            return false;
        }

        // Extract the credentials from the JSON response
        try {
            username = jsonData["data"]["data"]["POSTGRES_USERNAME"].asString();
            password = jsonData["data"]["data"]["POSTGRES_PASSWORD"].asString();
            database = jsonData["data"]["data"]["POSTGRES_DB"].asString();
            database_host = jsonData["data"]["data"]["POSTGRES_HOST"].asString();
            database_port = jsonData["data"]["data"]["POSTGRES_PORT"].asString();
        } catch (const std::exception& e) {
            std::cerr << "Error extracting credentials: " << e.what() << std::endl;
            return false;
        }

        return true;
    }

    return false;
}

// Function to initialize the database connection
bool initialiseDatabaseConnection(std::string& connection_string) {
    const char* vault_addr_env = std::getenv("VAULT_ADDR");
    const char* vault_token_env = std::getenv("VAULT_TOKEN");

    if (!vault_addr_env || !vault_token_env) {
        std::cerr << "Environment variables VAULT_ADDR and VAULT_TOKEN must be set." << std::endl;
        return false;
    }

    std::string vault_addr = vault_addr_env;
    std::string vault_token = vault_token_env;
    std::cout << "Vault Address: " << vault_addr << std::endl;
    std::cout << "Vault Token: " << vault_token << std::endl;
    std::string secret_path = "secret/data/postgres"; // Adjust this path based on your Vault setup
    std::string username, password, database, database_host, database_port;

    if (getPostgresCredentials(vault_addr, vault_token, secret_path, username, password, database, database_host, database_port)) {
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
        std::cerr << "Postgres credentials not found in Vault or environment variables" << std::endl;
        return false;
    }

    // Initialize database connection using the retrieved credentials
    connection_string = "dbname=" + database + " user=" + username + " password=" + password + " host=" + database_host + " port=" + database_port;
    return true;
}
