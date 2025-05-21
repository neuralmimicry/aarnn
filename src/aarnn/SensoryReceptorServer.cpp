// SensoryReceptorServer.cpp

#include <cstdlib>
#include <cerrno>
#include <climits>
#include <iostream>
#include "SensoryReceptorServer.h"
#include "StimuliData.h" // Definition of stimuli data structure

SensoryReceptorServer::SensoryReceptorServer() : running(false) {}

SensoryReceptorServer::~SensoryReceptorServer() {
    stopServer();
}

bool SensoryReceptorServer::initialise() {
    const char* portStr = std::getenv("SENSORY_SERVER_PORT");
    if (!portStr) {
        std::cerr << "Environment variable SENSORY_SERVER_PORT is not set." << std::endl;
        return false;
    }

    char* endPtr = nullptr;
    errno = 0;
    long portLong = std::strtol(portStr, &endPtr, 10);

    if (errno != 0 || endPtr == portStr || *endPtr != '\0') {
        std::cerr << "Invalid SENSORY_SERVER_PORT: '" << portStr << "' is not a valid number." << std::endl;
        return false;
    }

    if (portLong <= 0 || portLong > 65535) {
        std::cerr << "SENSORY_SERVER_PORT out of valid TCP port range: " << portLong << std::endl;
        return false;
    }

    server_port = static_cast<int>(portLong);

    networkServer = std::make_unique<AsyncNetworkServer>(server_port);
    if (!networkServer->initialise()) {
        std::cerr << "Failed to initialise network server." << std::endl;
        server_initialised = false;
    }
    else {
        server_initialised = true;
    }

    return server_initialised;
}

bool SensoryReceptorServer::startServer() {

    if (!server_initialised) {
        std::cerr << "Server not initialised. Cannot start." << std::endl;
        return false;
    }

    networkServer->setOnMessage([this](int clientId, const std::string& message) {
        processStimuliData(message);
    });

    if (!networkServer->start()) {
        std::cerr << "Failed to start AsyncNetworkServer." << std::endl;
        return false;
    }

    running = true;
    return true;
}

void SensoryReceptorServer::stopServer() {
    running = false;
    networkServer->stop();
}

void SensoryReceptorServer::registerReceptors(const std::string& receptorType,
                                              const std::vector<std::shared_ptr<SensoryReceptor>>& receptors) {
    receptorMap[receptorType] = receptors;
}

void SensoryReceptorServer::processStimuliData(const std::string& data) {
    StimuliData stimuli = deserializeStimuliData(data);
// Allow prefix matching: e.g., "Visual:LeftCamera" → "Visual"
    std::string baseType = stimuli.receptorType;
    auto colonPos = baseType.find(':');
    if (colonPos != std::string::npos) {
        baseType = baseType.substr(0, colonPos);
    }

    auto it = receptorMap.find(baseType);
    if (it != receptorMap.end()) {
        auto& receptors = it->second;
        for (size_t i = 0; i < receptors.size() && i < stimuli.values.size(); ++i) {
            receptors[i]->stimulate(stimuli.values[i]);
        }
    } else {
        std::cerr << "Unknown receptor type: " << stimuli.receptorType << std::endl;
    }
}
