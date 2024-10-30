// NetworkClient.cpp

#include "NetworkClient.h"
#include <iostream>

NetworkClient::NetworkClient(const std::string& host, unsigned short port)
        : host_(host), port_(port), socket_(std::make_unique<boost::asio::ip::tcp::socket>(io_service_))
{
}

NetworkClient::~NetworkClient() {
    disconnect();
}

bool NetworkClient::connect() {
    try {
        boost::asio::ip::tcp::resolver resolver(io_service_);
        boost::asio::ip::tcp::resolver::query query(host_, std::to_string(port_));
        boost::asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

        boost::asio::connect(*socket_, endpoint_iterator);
    }
    catch (const std::exception& e) {
        std::cerr << "NetworkClient::connect - Exception: " << e.what() << std::endl;
        return false;
    }
    return true;
}

bool NetworkClient::sendData(const std::string& data) {
    try {
        // Send data length first
        uint32_t dataLength = htonl(static_cast<uint32_t>(data.size()));
        boost::asio::write(*socket_, boost::asio::buffer(&dataLength, sizeof(dataLength)));

        // Send the actual data
        boost::asio::write(*socket_, boost::asio::buffer(data));
    }
    catch (const std::exception& e) {
        std::cerr << "NetworkClient::sendData - Exception: " << e.what() << std::endl;
        return false;
    }
    return true;
}

void NetworkClient::disconnect() {
    if (socket_ && socket_->is_open()) {
        boost::system::error_code ec;
        socket_->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        socket_->close(ec);
    }
}
