// SensoryReceptorServer.h

#include "SensoryReceptor.h"
#include <thread>
#include <atomic>
#include <map>
#include "NetworkServer.h" // Custom class for network communication

class SensoryReceptorServer {
public:
    SensoryReceptorServer();
    ~SensoryReceptorServer();

    bool startServer(int port);
    void stopServer();

    void registerReceptors(const std::string& receptorType,
                           const std::vector<std::shared_ptr<SensoryReceptor>>& receptors);

private:
    std::unique_ptr<NetworkServer> networkServer;
    std::atomic<bool> running;
    std::thread serverThread;

    // Map receptor type to receptors
    std::map<std::string, std::vector<std::shared_ptr<SensoryReceptor>>> receptorMap;

    void serverLoop();
    void handleClientConnection(int clientSocket);
    void processStimuliData(const std::string& data);
};
