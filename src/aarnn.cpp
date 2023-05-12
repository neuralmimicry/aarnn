/**
 * @file
 * @brief This file contains the implementation of a neuronal network simulation.
 */
#include <iostream>
#include <vector>
#include <list>
#include <algorithm>
#include <thread>
#include <mutex>
#include <atomic>
#include <cmath>
#include <pqxx/pqxx>
#include <stdexcept>
#include <fstream>
#include <string>
#include <map>
#include <set>
#include "aarnn.h"


// Function declarations

/**
 * @brief Read configuration files and return a map of key-value pairs.
 * @param filenames Vector of filenames to read the configuration from.
 * @return A map containing the key-value pairs from the configuration files.
 */
 std::map<std::string, std::string> read_config(const std::vector<std::string> &filenames) {
    std::map<std::string, std::string> config;

    for (const auto &filename : filenames) {
        std::ifstream file(filename);
        std::string line;

        while (std::getline(file, line)) {
            std::string key, value;
            std::istringstream line_stream(line);
            std::getline(line_stream, key, '=');
            std::getline(line_stream, value);
            config[key] = value;
        }
    }

    return config;
}

/**
 * @brief Build a connection string for the database using the configuration map.
 * @param config A map containing the key-value pairs from the configuration files.
 * @return A connection string containing the configuration for the database.
 */
std::string build_connection_string(const std::map<std::string, std::string> &config) {
    std::string connection_string;

    // List of allowed connection parameters
    std::set<std::string> allowed_params = {"host", "port", "user", "password", "dbname"};

    for (const auto &entry : config) {
        if (allowed_params.count(entry.first) > 0) {
            connection_string += entry.first + "=" + entry.second + " ";
        }
    }

    return connection_string;
}

// Logger class to write log messages to a file
class Logger {
public:
    /**
     * @brief Construct a new Logger object.
     * @param filename The name of the file to write log messages to.
     */

    explicit Logger(const std::string &filename) : log_file(filename, std::ofstream::out | std::ofstream::app) {}

    /**
     * @brief Destructor. Closes the log file.
     */
    ~Logger() {
        log_file.close();
    }

    /**
     * @brief Stream insertion operator to write messages to the log file.
     * @tparam T Type of the message.
     * @param msg The message to log.
     * @return Reference to the Logger object.
     */
    template<typename T>
    Logger &operator<<(const T &msg) {
        log_file << msg;
        log_file.flush();
        return *this;
    }

    /**
     * @brief Stream insertion operator overload for std::endl.
     * @param pf Pointer to the std::endl function.
     * @return Reference to the Logger object.
     */
    Logger &operator<<(std::ostream& (*pf)(std::ostream&)) {
        log_file << pf;
        log_file.flush();
        return *this;
    }

private:
    /**
     * @brief Output file stream to write log messages.
     */
    std::ofstream log_file;
};

// Add a new class to store neuron parameters and access the database
class NeuronParameters {
public:
    NeuronParameters() {
        // Access the database and load neuron parameters
    }

    /**
    * @brief Getter function for dendrite propagation rate.
    * @return The dendrite propagation rate.
    */
    // double getDendritePropagationRate() const;

private:
    /**
     * @brief Neuron parameter: dendrite propagation rate.
     */
    // double dendritePropagationRate;
};

/**
 * @class Position
 * @brief Class representing (x, y, z) coordinates in 3D space.
 */
class Position {
public:
    Position(double x, double y, double z) : x(x), y(y), z(z) {}

    // Function to calculate the Euclidean distance between two positions
    static double distance(const Position& p1, const Position& p2) {
        return std::sqrt(std::pow(p1.x - p2.x, 2) +
                         std::pow(p1.y - p2.y, 2) +
                         std::pow(p1.z - p2.z, 2));
    }

    double x, y, z;
};

// Neuron component base class
class NeuronComponent {
public:
    explicit NeuronComponent(const Position& position) : position(position) {}
    virtual ~NeuronComponent() = default;

    [[nodiscard]] const Position& getPosition() const {
        return position;
    }

    // Add other relevant member functions and data members for different components
    virtual double getPropagationRate() = 0;

private:
    Position position;
};

class Shape3D {
public:
    Shape3D() = default;
    ~Shape3D() = default;

    // Add methods and data members to represent the 3D shape, e.g., vertices, faces, etc.
};

// Neuron shape component base class
class NeuronShapeComponent {
public:
    NeuronShapeComponent() = default;
    virtual ~NeuronShapeComponent() = default;

    virtual void setShape(const Shape3D& newShape) { // Change the parameter name to 'newShape'
        this->shape = newShape;
    }

    [[nodiscard]] virtual const Shape3D& getShape() const {
        return shape;
    }

private:
    Shape3D shape{};
};

// Dendrite Bouton class
class DendriteBouton : public NeuronComponent, public NeuronShapeComponent {
public:
    explicit DendriteBouton(const Position& position) : NeuronComponent(position), NeuronShapeComponent() {}
    ~DendriteBouton() override = default;

    double getPropagationRate() override {
        // Implement the calculation based on the dendrite bouton properties
        return 0;
    }
};

// Dendrite class
class Dendrite : public NeuronComponent, public NeuronShapeComponent {
public:
    explicit Dendrite(const Position& position) : NeuronComponent(position), NeuronShapeComponent() {}
    ~Dendrite() override = default;

    void addBouton(double position, DendriteBouton* bouton) {
        boutons.push_back(std::make_pair(position, bouton));
    }

    [[nodiscard]] const std::vector<std::pair<double, DendriteBouton*>>& getBoutons() const {
        return boutons;
    }

    double getPropagationRate() override {
        double rate = 0;
        for (const auto& pair : boutons) {
            rate += pair.second->getPropagationRate();
        }
        return rate;
    }

private:
    std::vector<std::pair<double, DendriteBouton*>> boutons;
    DendriteBranch* branch;
};

// Dendrite branch class
class DendriteBranch : public NeuronComponent, public NeuronShapeComponent {
public:
    explicit DendriteBranch(const Position& position) : NeuronComponent(position), NeuronShapeComponent() {}
    ~DendriteBranch() override = default;

    void connectDendrites(Dendrite* dendrite1, Dendrite* dendrite2) {
        onwardDendrites[0] = dendrite1;
        onwardDendrites[1] = dendrite2;
    }

    [[nodiscard]] Dendrite* getOnwardDendrite(int index) const {
        return onwardDendrites[index];
    }

    double getPropagationRate() override {
        // Implement the calculation based on the dendrite branch properties
        return 0;
    }

private:
    Dendrite* onwardDendrites[2];
};

// Soma class
class Soma : public NeuronComponent, public NeuronShapeComponent {
public:
    explicit Soma(const Position& position) : NeuronComponent(position), NeuronShapeComponent() {}
    ~Soma() override = default;

    void addDendrite(Dendrite* dendrite) {
        dendrites.push_back(dendrite);
    }

    [[nodiscard]] const std::vector<Dendrite*>& getDendrites() const {
        return dendrites;
    }

    double getPropagationRate() override {
        double rate = 0;
        for (const auto& dendrite : dendrites) {
            rate += dendrite->getPropagationRate();
        }
        return rate;
    }

private:
    std::vector<Dendrite*> dendrites;
};

// Axon branch class
class AxonBranch : public NeuronComponent, public NeuronShapeComponent {
public:
    explicit AxonBranch(const Position& position) : NeuronComponent(position), NeuronShapeComponent() {}
    ~AxonBranch() override = default;

    double getPropagationRate() override {
        // Implement the calculation based on the axon branch properties
        return 0;
    }
};

// Axon class
class Axon : public NeuronComponent, public NeuronShapeComponent {
public:
    explicit Axon(const Position& position) : NeuronComponent(position), NeuronShapeComponent() {}
    ~Axon() override = default;

    void addBranch(AxonBranch* branch) {
        branches.push_back(branch);
    }

    [[nodiscard]] const std::vector<AxonBranch*>& getBranches() const {
        return branches;
    }

    double getPropagationRate() override {
        double rate = 0;
        for (const auto& branch : branches) {
            rate += branch->getPropagationRate();
        }
        return rate;
    }

private:
    std::vector<AxonBranch*> branches;
};

// Axon hillock class
class AxonHillock : public NeuronComponent, public NeuronShapeComponent {
public:
    explicit AxonHillock(const Position& position) : NeuronComponent(position), NeuronShapeComponent() {}
    ~AxonHillock() override = default;

    double getPropagationRate() override {
        // Implement the calculation based on the axon hillock properties
        return 0;
    }
};

// Synaptic bouton class
class SynapticBouton : public NeuronComponent, public NeuronShapeComponent {
public:
    explicit SynapticBouton(const Position& position) : NeuronComponent(position), NeuronShapeComponent() {}
    ~SynapticBouton() override = default;

    double getPropagationRate() override {
        // Implement the calculation based on the synapse properties
        return 0;
    }
};

// Synaptic gap class
class SynapticGap : public NeuronComponent, public NeuronShapeComponent {
public:
    explicit SynapticGap(const Position& position) : NeuronComponent(position), NeuronShapeComponent() {}
    ~SynapticGap() override = default;

    double getPropagationRate() override {
        // Implement the calculation based on the synapse properties
        return 0;
    }
};

/**
 * @brief Neuron class representing a neuron in the simulation.
 */
class Neuron {
public:
    /**
     * @brief Construct a new Neuron object.
     * @param dendrite Pointer to the Dendrite object of the neuron.
     * @param soma Pointer to the Soma object of the neuron.
     * @param axonHillock Pointer to the AxonHillock object of the neuron.
     * @param axon Pointer to the Axon object of the neuron.
     */
    Neuron(Dendrite* dendrite, Soma* soma, AxonHillock* axonHillock, Axon* axon)
        : dendrite(dendrite), soma(soma), axonHillock(axonHillock), axon(axon) {}

    /**
     * @brief Destroy the Neuron object.
     */
    ~Neuron() = default;

    /**
     * @brief Getter function for the soma of the neuron.
     * @return Pointer to the Soma object of the neuron.
     */
    Soma* getSoma() {
        return soma;
    }

    /**
     * @brief Getter function for the axon hillock of the neuron.
     * @return Pointer to the AxonHillock object of the neuron.
     */
    AxonHillock* getAxonHillock() {
        return axonHillock;
    }

    /**
     * @brief Getter function for the axon of the neuron.
     * @return Pointer to the Axon object of the neuron.
     */
    Axon* getAxon() {
        return axon;
    }

    /**
     * @brief Getter function for the dendrite of the neuron.
     * @return Pointer to the Dendrite object of the neuron.
     */
    Dendrite* getDendrite() {
        return dendrite;
    }

private:
    Soma* soma;
    AxonHillock* axonHillock;
    Axon* axon;
    Dendrite* dendrite;
};

// Global variables
std::atomic<double> totalPropagationRate(0.0);
std::mutex mtx;

/**
 * @brief Compute the propagation rate of a neuron.
 * @param neuron Pointer to the Neuron object for which the propagation rate is to be calculated.
 */
void computePropagationRate(Neuron* neuron) {
    double propagationRate = neuron->getDendrite()->getPropagationRate() +
                             neuron->getSoma()->getPropagationRate() +
                             neuron->getAxonHillock()->getPropagationRate() +
                             neuron->getAxon()->getPropagationRate();

    // Lock the mutex to safely update the totalPropagationRate
    {
        std::lock_guard<std::mutex> lock(mtx);
        totalPropagationRate = totalPropagationRate + propagationRate;
    }
}

/**
 * @brief Initialize the database, checking and creating the required table if needed.
 * @param conn Reference to the pqxx::connection object.
 */
void initialize_database(pqxx::connection& conn) {
    pqxx::work txn(conn);

    // Check if the "neurons" table exists
    pqxx::result result = txn.exec(
            "SELECT EXISTS ("
            "SELECT FROM pg_tables WHERE schemaname = 'public' AND tablename = 'neurons'"
            ");"
    );

    if (!result.empty() && !result[0][0].as<bool>()) {
        // Create the "neurons" table if it does not exist
        txn.exec(
                "CREATE TABLE neurons ("
                "id SERIAL PRIMARY KEY,"
                "propagation_rate REAL NOT NULL,"
                "neuron_type INTEGER NOT NULL,"
                "axon_length REAL NOT NULL"
                ");"
        );

        txn.commit();
        std::cout << "Created table 'neurons'." << std::endl;
    } else {
        std::cout << "Table 'neurons' already exists." << std::endl;
    }
}

/**
 * @brief Main function to initialize, run, and finalize the neuron network simulation.
 * @return int Status code (0 for success, non-zero for failures).
 */
int main() {
    // Initialize logger
    Logger logger("errors.log");

    try {
        // Read the database connection configuration and simulation configuration
        std::vector<std::string> config_filenames = {"db_connection.conf", "simulation.conf"};
        auto config = read_config(config_filenames);
        std::string connection_string = build_connection_string(config);

        // Get the number of neurons from the configuration
        int num_neurons = std::stoi(config["num_neurons"]);

        // Connect to PostgreSQL
        pqxx::connection conn(connection_string);

        // Initialize the database (check and create the required table if needed)
        initialize_database(conn);
        // Start a transaction
        pqxx::work txn(conn);
        // Set the transaction isolation level
        txn.exec("SET TRANSACTION ISOLATION LEVEL SERIALIZABLE;");
        txn.exec("SET lock_timeout = '5s';");

        // Create a single instance of NeuronParameters to access the database
        NeuronParameters params;

        // Create shared instances of neuron components
        Dendrite sharedDendrite(Position(0, 0, 0));
        Soma sharedSoma(Position(0, 0, 0));
        AxonHillock sharedAxonHillock(Position(0, 0, 0));
        Axon sharedAxon(Position(0, 0, 0));

        // Create a list of neurons
        std::vector<Neuron*> neurons;
        neurons.reserve(num_neurons);
        for (int i = 0; i < num_neurons; ++i) {
            neurons.push_back(new Neuron(&sharedDendrite, &sharedSoma, &sharedAxonHillock, &sharedAxon));
        }

        // Add multiple dendrite branches and axon branches to the neurons
        for (Neuron* neuron : neurons) {
        // Add dendrite branches to the dendrite
        for (int i = 0; i < 3; ++i) {
        auto* dendriteBranch = new DendriteBranch(Position(0, 0, 0));
        neuron->getDendrite()->addBranch(dendriteBranch);
        }
        // Add axon branches to the axon
        for (int i = 0; i < 3; ++i) {
            auto* axonBranch = new AxonBranch(Position(0, 0, 0));
            neuron->getAxon()->addBranch(axonBranch);
        }
    }

        // Calculate the propagation rate in parallel
        std::vector<std::thread> threads;
        threads.reserve(neurons.size());  // Pre-allocate the capacity before the loop

        for (Neuron* neuron : neurons) {
            threads.emplace_back(computePropagationRate, neuron);
        }
        // Join the threads
        for (std::thread& t : threads) {
            t.join();
        }

        // Get the total propagation rate
        double propagationRate = totalPropagationRate.load();

            std::cout << "The propagation rate is " << propagationRate << std::endl;

            // Add any additional functionality for virtual volume constraint and relaxation here
            // Save the propagation rate to the database if it's valid (not null)
            if (propagationRate != 0) {
                std::string query = "INSERT INTO neurons (propagation_rate) VALUES (" + std::to_string(propagationRate) + ")";
                txn.exec(query);
                txn.commit();
            } else {
                throw std::runtime_error("The propagation rate is not valid. Skipping database insertion.");
            }

    // Clean up memory
        for (Neuron* neuron : neurons) {
            // Delete dendrite branches
            for (DendriteBranch* branch : neuron->getDendrite()->getBranches()) {
                delete branch;
            }

            // Delete axon branches
            for (AxonBranch* branch : neuron->getAxon()->getBranches()) {
                delete branch;
            }

            delete neuron;
        }

    } catch (const std::exception &e) {
        logger << "Error: " << e.what() << std::endl;
        std::cerr << "Error: " << e.what() << std::endl;
    } catch (...) {
        logger << "Error: Unknown exception occurred." << std::endl;
        std::cerr << "Error: Unknown exception occurred." << std::endl;
    }

    return 0;
}
