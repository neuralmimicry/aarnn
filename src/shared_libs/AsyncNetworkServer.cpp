// AsyncNetworkServer.cpp
#include "AsyncNetworkServer.h"
#include <iostream>
#include <netinet/in.h>
#include <cstring>

AsyncNetworkServer::AsyncNetworkServer(int port) : port(port) {}

AsyncNetworkServer::~AsyncNetworkServer() {
    stop();
}

bool AsyncNetworkServer::initialise() {
    try {
        acceptor = std::make_unique<boost::asio::ip::tcp::acceptor>(ioContext);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "AsyncNetworkServer::initialise - Exception: " << e.what() << std::endl;
        return false;
    }
}

bool AsyncNetworkServer::start() {
    try {
        acceptor = std::make_unique<boost::asio::ip::tcp::acceptor>(
                ioContext,
                boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)
        );

        running = true;
        doAccept();

        ioThread = std::thread([this]() { ioContext.run(); });
        return true;
    } catch (const std::exception& e) {
        std::cerr << "AsyncNetworkServer::start - Exception: " << e.what() << std::endl;
        return false;
    }
}

void AsyncNetworkServer::stop() {
    running = false;
    ioContext.stop();
    if (ioThread.joinable()) {
        ioThread.join();
    }

    std::lock_guard<std::mutex> lock(clientMutex);
    clients.clear();
}

void AsyncNetworkServer::setOnMessage(std::function<void(int, const std::string&)> callback) {
    onMessage = std::move(callback);
}

void AsyncNetworkServer::doAccept() {
    auto socket = std::make_shared<boost::asio::ip::tcp::socket>(ioContext);
    acceptor->async_accept(*socket, [this, socket](const boost::system::error_code& ec) {
        if (!ec && running) {
            int clientId = nextClientId++;
            ClientSession session;
            session.socket = socket;
            session.buffer.resize(sizeof(uint32_t));

            {
                std::lock_guard<std::mutex> lock(clientMutex);
                clients[clientId] = session;
            }

            doRead(clientId);
        }

        if (running) {
            doAccept(); // Accept next client
        }
    });
}

void AsyncNetworkServer::doRead(int clientId) {
    std::lock_guard<std::mutex> lock(clientMutex);
    auto it = clients.find(clientId);
    if (it == clients.end()) return;

    auto& session = it->second;
    auto socket = session.socket;

    // Step 1: Read message length (4 bytes)
    boost::asio::async_read(*socket, boost::asio::buffer(session.buffer.data(), sizeof(uint32_t)),
                            [this, clientId](const boost::system::error_code& ec, std::size_t) {
                                if (ec) {
                                    closeClient(clientId);
                                    return;
                                }

                                std::lock_guard<std::mutex> lock(clientMutex);
                                auto& session = clients[clientId];
                                std::memcpy(&session.expectedLength, session.buffer.data(), sizeof(uint32_t));
                                session.expectedLength = ntohl(session.expectedLength);
                                session.buffer.resize(session.expectedLength);

                                // Step 2: Read message body
                                boost::asio::async_read(*session.socket, boost::asio::buffer(session.buffer.data(), session.expectedLength),
                                                        [this, clientId](const boost::system::error_code& ec, std::size_t) {
                                                            if (ec) {
                                                                closeClient(clientId);
                                                                return;
                                                            }

                                                            std::string message(clients[clientId].buffer.begin(), clients[clientId].buffer.end());
                                                            if (onMessage) onMessage(clientId, message);

                                                            doRead(clientId); // Wait for next message
                                                        });
                            });
}

void AsyncNetworkServer::doWrite(int clientId, const std::string& message) {
    std::lock_guard<std::mutex> lock(clientMutex);
    auto it = clients.find(clientId);
    if (it == clients.end()) return;

    auto socket = it->second.socket;
    uint32_t len = htonl(static_cast<uint32_t>(message.size()));
    std::vector<boost::asio::const_buffer> buffers = {
            boost::asio::buffer(&len, sizeof(len)),
            boost::asio::buffer(message)
    };

    boost::asio::async_write(*socket, buffers,
                             [clientId](const boost::system::error_code& ec, std::size_t) {
                                 if (ec) {
                                     std::cerr << "Write failed for client " << clientId << ": " << ec.message() << std::endl;
                                 }
                             });
}

void AsyncNetworkServer::closeClient(int clientId) {
    std::lock_guard<std::mutex> lock(clientMutex);
    clients.erase(clientId);
    std::cout << "Client " << clientId << " disconnected." << std::endl;
}
