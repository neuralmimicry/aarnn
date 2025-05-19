// AsyncNetworkClient.cpp
#include "AsyncNetworkClient.h"
#include <iostream>
#include <netinet/in.h>
#include <cstring>

AsyncNetworkClient::AsyncNetworkClient(const std::string& host, unsigned short port)
        : host(host), port(port) {}

AsyncNetworkClient::~AsyncNetworkClient() {
    disconnect();
}

bool AsyncNetworkClient::connect() {
    try {
        resolver = std::make_unique<boost::asio::ip::tcp::resolver>(ioContext);
        socket = std::make_unique<boost::asio::ip::tcp::socket>(ioContext);

        doConnect();
        ioThread = std::thread([this]() { ioContext.run(); });
        return true;
    } catch (const std::exception& e) {
        std::cerr << "AsyncNetworkClient::connect - Exception: " << e.what() << std::endl;
        return false;
    }
}

void AsyncNetworkClient::disconnect() {
    ioContext.stop();
    if (ioThread.joinable()) {
        ioThread.join();
    }
    if (socket && socket->is_open()) {
        boost::system::error_code ec;
        socket->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        socket->close(ec);
    }
}

void AsyncNetworkClient::setOnMessage(std::function<void(const std::string&)> callback) {
    onMessage = std::move(callback);
}

void AsyncNetworkClient::send(const std::string& message) {
    std::lock_guard<std::mutex> lock(writeMutex);
    doWrite(message);
}

void AsyncNetworkClient::doConnect() {
    auto endpoints = resolver->resolve(host, std::to_string(port));
    boost::asio::async_connect(*socket, endpoints,
                               [this](const boost::system::error_code& ec, const boost::asio::ip::tcp::endpoint&) {
                                   if (!ec) {
                                       doReadHeader();
                                   } else {
                                       std::cerr << "AsyncNetworkClient::doConnect - Failed: " << ec.message() << std::endl;
                                   }
                               });
}

void AsyncNetworkClient::doReadHeader() {
    readBuffer.resize(sizeof(uint32_t));
    boost::asio::async_read(*socket, boost::asio::buffer(readBuffer),
                            [this](const boost::system::error_code& ec, std::size_t) {
                                if (ec) return;

                                uint32_t length;
                                std::memcpy(&length, readBuffer.data(), sizeof(uint32_t));
                                length = ntohl(length);
                                doReadBody(length);
                            });
}

void AsyncNetworkClient::doReadBody(std::size_t length) {
    readBuffer.resize(length);
    boost::asio::async_read(*socket, boost::asio::buffer(readBuffer),
                            [this](const boost::system::error_code& ec, std::size_t) {
                                if (!ec) {
                                    std::string message(readBuffer.begin(), readBuffer.end());
                                    if (onMessage) onMessage(message);
                                    doReadHeader(); // Keep reading
                                }
                            });
}

void AsyncNetworkClient::doWrite(const std::string& message) {
    uint32_t len = htonl(static_cast<uint32_t>(message.size()));
    std::vector<boost::asio::const_buffer> buffers = {
            boost::asio::buffer(&len, sizeof(len)),
            boost::asio::buffer(message)
    };

    boost::asio::async_write(*socket, buffers,
                             [](const boost::system::error_code& ec, std::size_t) {
                                 if (ec) {
                                     std::cerr << "AsyncNetworkClient::doWrite - Write failed: " << ec.message() << std::endl;
                                 }
                             });
}
