#ifndef DB_H_INCLUDED
#define DB_H_INCLUDED

#include <iostream>
#include <memory>
#include <pqxx/pqxx>
#include <spdlog/spdlog.h>
#include <sstream>
#include <string>

class DB
{
    public:
    /**
     * @brief Construct a new DB object
     *
     * @param user database user with access to the given database 'dbname'
     * @param password password for the user
     * @param port port on which postgres is serving requests
     * @param host host/IP of the database
     * @param dbname database name
     */
    DB(const std::string& user,
       const std::string& password,
       const std::string& port,
       const std::string& host   = "localhost",
       const std::string& dbname = "neurons")
    : user_(user)
    , password_(password)
    , port_(port)
    , host_(host)
    , dbname_(dbname)
    {
        auto conn_string = connection_string();
        spdlog::info("Attempting to connect to PostGreSQL to '{}'", dbname_);

        try
        {
            pConn_ = std::make_shared<pqxx::connection>(conn_string);
            if(pConn_)
            {
                spdlog::info("Connected.");
            }
            else
            {
                throw std::runtime_error("unknown error when trying to establish pqxx::connection");
            }
        }
        catch(const std::exception& e)
        {
            std::stringstream ss;
            ss << __PRETTY_FUNCTION__ << ": Failed to connect to '" << dbname_ << "' - " << e.what();
            auto error = std::runtime_error(ss.str());
            spdlog::error(error.what());
            throw error;
        }

        // Initialize the database (check and create the required table if needed)
        init();
    }

    /**
     * @brief Initialize the database, checking and creating the required table if needed.
     *
     * @param force if true, then re-initialize the database if it exists
     */
    void init(bool force = false)
    {
        if(force || !isInitalised_)
        {
            startTransaction();

            // Check if the "neurons" table exists
            exec(R"(
            DROP TABLE IF EXISTS
                neurons,
                somas,
                axonhillocks,
                axons,
                axons_hillock,
                axonboutons,
                synapticgaps, 
                dendritebranches_soma,
                dendritebranches,
                dendrites,
                dendriteboutons,
                axonbranches CASCADE;
            COMMIT;
            )");

            // Check if the "neurons" table exists
            pqxx::result result = exec(R"(
                                  SELECT EXISTS (
                                    SELECT FROM pg_tables WHERE schemaname = 'public' AND tablename = 'neurons'
                                    );
                                    )");
            commitTransaction();

            startTransaction();
            if(!result.empty() && !result[0][0].as<bool>())
            {
                // Create the "neurons" table if it does not exist
                exec(R"(
                CREATE TABLE neurons (
                     neuron_id SERIAL PRIMARY KEY,
                     x REAL NOT NULL,
                     y REAL NOT NULL,
                     z REAL NOT NULL,
                     propagation_rate REAL,
                     neuron_type INTEGER,
                     axon_length REAL
                     );

                     CREATE TABLE somas (
                     soma_id SERIAL PRIMARY KEY,
                     neuron_id INTEGER REFERENCES neurons(neuron_id),
                     x REAL NOT NULL,
                     y REAL NOT NULL,
                     z REAL NOT NULL
                     );

                     CREATE TABLE axonhillocks (
                     axon_hillock_id SERIAL PRIMARY KEY,
                     soma_id INTEGER REFERENCES somas(soma_id),
                     x REAL NOT NULL,
                     y REAL NOT NULL,
                     z REAL NOT NULL
                     );

                     CREATE TABLE axons_hillock (
                     axon_id SERIAL PRIMARY KEY,
                     axon_hillock_id INTEGER,
                     x REAL NOT NULL,
                     y REAL NOT NULL,
                     z REAL NOT NULL
                     );

                     CREATE TABLE axons (
                     axon_id SERIAL PRIMARY KEY,
                     axon_hillock_id INTEGER REFERENCES axonhillocks(axon_hillock_id),
                     x REAL NOT NULL,
                     y REAL NOT NULL,
                     z REAL NOT NULL
                     );

                     ALTER TABLE axons_hillock 
                     ADD CONSTRAINT fk_axons_hillock_axon_id 
                     FOREIGN KEY (axon_id) REFERENCES axons(axon_id);

                     CREATE TABLE axonboutons (
                     axon_bouton_id SERIAL PRIMARY KEY,
                     axon_id INTEGER REFERENCES axons(axon_id),
                     x REAL NOT NULL,
                     y REAL NOT NULL,
                     z REAL NOT NULL
                     );

                     CREATE TABLE synapticgaps (
                     synaptic_gap_id SERIAL PRIMARY KEY,
                     axon_bouton_id INTEGER REFERENCES axonboutons(axon_bouton_id),
                     x REAL NOT NULL,
                     y REAL NOT NULL,
                     z REAL NOT NULL
                     );

                     CREATE TABLE axonbranches (
                     axon_branch_id SERIAL PRIMARY KEY,
                     axon_id INTEGER REFERENCES axons(axon_id),
                     x REAL NOT NULL,
                     y REAL NOT NULL,
                     z REAL NOT NULL
                     );

                     CREATE TABLE dendritebranches_soma (
                     dendrite_branch_id SERIAL PRIMARY KEY,
                     soma_id INTEGER REFERENCES somas(soma_id),
                     x REAL NOT NULL,
                     y REAL NOT NULL,
                     z REAL NOT NULL
                     );

                     CREATE TABLE dendrites (
                     dendrite_id SERIAL PRIMARY KEY,
                     dendrite_branch_id INTEGER,
                     x REAL NOT NULL,
                     y REAL NOT NULL,
                     z REAL NOT NULL
                     );

                     CREATE TABLE dendritebranches (
                     dendrite_branch_id SERIAL PRIMARY KEY,
                     dendrite_id INTEGER REFERENCES dendrites(dendrite_id),
                     x REAL NOT NULL,
                     y REAL NOT NULL,
                     z REAL NOT NULL
                     );

                     ALTER TABLE dendritebranches 
                     ADD CONSTRAINT fk_dendritebranches_dendrite_id 
                     FOREIGN KEY (dendrite_id) REFERENCES dendrites(dendrite_id);

                     CREATE TABLE dendriteboutons (
                     dendrite_bouton_id SERIAL PRIMARY KEY,
                     dendrite_id INTEGER REFERENCES dendrites(dendrite_id),
                     x REAL NOT NULL,
                     y REAL NOT NULL,
                     z REAL NOT NULL
                     );
                     )");

                spdlog::info("Created all tables.");
            }
            else
            {
                spdlog::warn("Tables already exists.");
            }
            commitTransaction();
        }
        isInitalised_ = true;
    }

    /**
     * @brief Start a transaction.
     */
    void startTransaction()
    {
        if(pTransaction_)
            return;
        if(!pConn_)
        {
            std::stringstream ss;
            ss << __PRETTY_FUNCTION__ << ": No database connection";
            auto error = std::runtime_error(ss.str());
            spdlog::error(error.what());
            throw error;
        }
        pTransaction_ = std::make_shared<pqxx::work>(*pConn_);
    }

    /**
     * @brief Execute a query.
     *
     * @param statement a string with the statement(s) to execute.
     * @return pqxx::result the result of the query
     */
    pqxx::result exec(const std::string& statement) const
    {
        if(pTransaction_)
        {
            return pTransaction_->exec(statement.c_str());
        }

        std::stringstream ss;
        ss << __PRETTY_FUNCTION__ << ": No database transaction open when trying to execute '" << statement << "'";
        auto error = std::runtime_error(ss.str());
        spdlog::error(error.what());
        throw error;
    }

    /**
     * @brief Commit the transaction.
     */
    void commitTransaction()
    {
        if(!pTransaction_)
        {
            std::stringstream ss;
            ss << __PRETTY_FUNCTION__ << ": No database transaction open when trying to commit";
            auto error = std::runtime_error(ss.str());
            spdlog::error(error.what());
            throw error;
        }
        pTransaction_->commit();
    }

    private:
    /**
     * @brief Build a connection string for the database using the configuration map.
     * @return A connection string containing the configuration for the database.
     */
    std::string connection_string() const
    {
        std::stringstream ss;
        ss << "host=" << host_ << " port=" << port_ << " user=" << user_ << " password=" << password_
           << " dbname=" << dbname_;

        return ss.str();
    }

    std::string                       user_;
    std::string                       password_;
    std::string                       port_;
    std::string                       host_;
    std::string                       dbname_;
    std::shared_ptr<pqxx::connection> pConn_;
    std::shared_ptr<pqxx::work>       pTransaction_;
    bool                              isInitalised_ = false;
};

#endif  // DB_H_INCLUDED
