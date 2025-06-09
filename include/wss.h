#ifndef AARNN_WSS_H
#define AARNN_WSS_H

#pragma once
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <mutex>
#include <set>
#include <string>

/// A minimal thread-safe WebSocket server using WebSocket++
class WebSocketServer {
public:
    WebSocketServer();
    /// Run the server on the given TCP port (blocking)
    void run(uint16_t port);
    /// Broadcast a text message to all connected clients
    void broadcast(const std::string& message);

private:
    using server_t = websocketpp::server<websocketpp::config::asio>;
    server_t ws_server_;
    std::set<websocketpp::connection_hdl, std::owner_less<websocketpp::connection_hdl>> connections_;
    std::mutex conn_mutex_;

    void on_open(websocketpp::connection_hdl hdl);
    void on_close(websocketpp::connection_hdl hdl);
};

#endif // AARNN_WSS_H
