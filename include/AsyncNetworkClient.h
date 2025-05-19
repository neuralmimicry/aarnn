// AsyncNetworkClient.h
#pragma once

#include <boost/asio.hpp>
#include <memory>
#include <thread>
#include <mutex>
#include <functional>
#include <string>

class AsyncNetworkClient {
public:
    AsyncNetworkClient(const std::string& host, unsigned short port);
    ~AsyncNetworkClient();

    bool connect();
    void disconnect();
    void send(const std::string& message);
    void setOnMessage(std::function<void(const std::string&)> callback);

private:
    void doConnect();
    void doReadHeader();
    void doReadBody(std::size_t length);
    void doWrite(const std::string& message);

    std::string host;
    unsigned short port;

    boost::asio::io_context ioContext;
    std::unique_ptr<boost::asio::ip::tcp::socket> socket;
    std::unique_ptr<boost::asio::ip::tcp::resolver> resolver;
    std::thread ioThread;

    std::mutex writeMutex;
    std::function<void(const std::string&)> onMessage;

    std::vector<char> readBuffer;
};

