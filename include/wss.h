#ifndef AARNN_WSS_H
#define AARNN_WSS_H

#include <set>
#include <mutex>
#include <string>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

typedef websocketpp::server<websocketpp::config::asio> wss_server;

class WebSocketServer {
public:
    WebSocketServer();

    void run(uint16_t port);
    void broadcast(const std::string& message);

private:
    void on_open(websocketpp::connection_hdl hdl);
    void on_close(websocketpp::connection_hdl hdl);

    wss_server ws_server_;
    std::set<websocketpp::connection_hdl, std::owner_less<websocketpp::connection_hdl>> connections_;
    std::mutex connection_mutex_;
};

#endif // AARNN_WSS_H
