// SensoryReceptorServer.h

#include "SensoryReceptor.h"
#include <thread>
#include <atomic>
#include <map>
#include "AsyncNetworkServer.h" // Custom class for network communication

class SensoryReceptorServer {
public:
    SensoryReceptorServer();
    ~SensoryReceptorServer();

    // Initialise the server
    bool initialise();
    bool startServer();
    void stopServer();

    void registerReceptors(const std::string& receptorType,
                           const std::vector<std::shared_ptr<SensoryReceptor>>& receptors);

private:
    std::unique_ptr<AsyncNetworkServer> networkServer{};
    std::atomic<bool> running;
    int server_port;
    bool server_initialised = false;
    // Map receptor type to receptors
    std::map<std::string, std::vector<std::shared_ptr<SensoryReceptor>>> receptorMap;

    void processStimuliData(const std::string& data);
};
