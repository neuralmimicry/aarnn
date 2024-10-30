// SensoryReceptorServer.cpp

#include "SensoryReceptorServer.h"
#include "StimuliData.h" // Definition of stimuli data structure

SensoryReceptorServer::SensoryReceptorServer() : running(false) {}

SensoryReceptorServer::~SensoryReceptorServer() {
    stopServer();
}

bool SensoryReceptorServer::startServer(int port) {
    networkServer = std::make_unique<NetworkServer>(port);
    if (!networkServer->start()) {
        std::cerr << "Failed to start network server." << std::endl;
        return false;
    }

    running = true;
    serverThread = std::thread(&SensoryReceptorServer::serverLoop, this);

    return true;
}

void SensoryReceptorServer::stopServer() {
    running = false;
    if (serverThread.joinable()) {
        serverThread.join();
    }
    networkServer->stop();
}

void SensoryReceptorServer::registerReceptors(const std::string& receptorType,
                                              const std::vector<std::shared_ptr<SensoryReceptor>>& receptors) {
    receptorMap[receptorType] = receptors;
}

void SensoryReceptorServer::serverLoop() {
    while (running.load()) {
        int clientSocket = networkServer->acceptConnection();
        if (clientSocket >= 0) {
            std::thread(&SensoryReceptorServer::handleClientConnection, this, clientSocket).detach();
        }
    }
}

void SensoryReceptorServer::handleClientConnection(int clientSocket) {
    std::string data;
    while (networkServer->receiveData(clientSocket, data)) {
        processStimuliData(data);
    }
    networkServer->closeConnection(clientSocket);
}

void SensoryReceptorServer::processStimuliData(const std::string& data) {
    StimuliData stimuli = deserializeStimuliData(data);
    auto it = receptorMap.find(stimuli.receptorType);
    if (it != receptorMap.end()) {
        auto& receptors = it->second;
        for (size_t i = 0; i < receptors.size() && i < stimuli.values.size(); ++i) {
            receptors[i]->stimulate(stimuli.values[i]);
        }
    } else {
        std::cerr << "Unknown receptor type: " << stimuli.receptorType << std::endl;
    }
}
