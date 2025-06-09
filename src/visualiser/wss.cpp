#include "wss.h"
#include <iostream>

WebSocketServer::WebSocketServer() {
    ws_server_.init_asio();
    ws_server_.set_open_handler([this](websocketpp::connection_hdl hdl) {
        std::lock_guard<std::mutex> lock(conn_mutex_);
        connections_.insert(hdl);
        std::cout << "[WS] Client connected\n";
    });
    ws_server_.set_close_handler([this](websocketpp::connection_hdl hdl) {
        std::lock_guard<std::mutex> lock(conn_mutex_);
        connections_.erase(hdl);
        std::cout << "[WS] Client disconnected\n";
    });
}

void WebSocketServer::run(uint16_t port) {
    ws_server_.listen(port);
    ws_server_.start_accept();
    ws_server_.run();
}

void WebSocketServer::broadcast(const std::string& message) {
    std::lock_guard<std::mutex> lock(conn_mutex_);
    for (auto hdl : connections_) {
        ws_server_.send(hdl, message, websocketpp::frame::opcode::text);
    }
}
