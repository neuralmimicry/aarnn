// AsyncNetworkServer.h
#pragma once

#include <boost/asio.hpp>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <functional>
#include <string>

class AsyncNetworkServer {
public:
    explicit AsyncNetworkServer(int port);
    ~AsyncNetworkServer();

    bool initialise();
    bool start();
    void stop();

    // Callback invoked when a complete message is received from a client
    void setOnMessage(std::function<void(int, const std::string&)> callback);

private:
    void doAccept();
    void doRead(int clientId);
    void doWrite(int clientId, const std::string& message);
    void closeClient(int clientId);

    struct ClientSession {
        std::shared_ptr<boost::asio::ip::tcp::socket> socket;
        std::vector<char> buffer;
        uint32_t expectedLength = 0;
    };

    int port;
    bool running = false;

    boost::asio::io_context ioContext;
    std::unique_ptr<boost::asio::ip::tcp::acceptor> acceptor;
    std::thread ioThread;

    std::mutex clientMutex;
    std::unordered_map<int, ClientSession> clients;
    std::function<void(int, const std::string&)> onMessage;

    int nextClientId = 1;
};
