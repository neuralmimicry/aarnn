// include/NetworkServer.h
#pragma once

#include <string>

class NetworkServer {
public:
    explicit NetworkServer(int port);
    ~NetworkServer();

    bool start();
    void stop();

    int acceptConnection();
    bool receiveData(int clientSocket, std::string& data);
    void closeConnection(int clientSocket);

private:
    int port;
    bool running;
};
