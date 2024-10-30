// NetworkClient.h

#pragma once

#include <string>
#include <memory>
#include <boost/asio.hpp>

class NetworkClient {
public:
    NetworkClient(const std::string& host, unsigned short port);
    ~NetworkClient();

    bool connect();
    bool sendData(const std::string& data);
    void disconnect();

private:
    std::string host_;
    unsigned short port_;

    boost::asio::io_service io_service_;
    std::unique_ptr<boost::asio::ip::tcp::socket> socket_;
};
