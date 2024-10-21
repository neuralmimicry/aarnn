#include "wss.h"

WebSocketServer::WebSocketServer() {
    // Initialize Asio Transport
    ws_server_.init_asio();

    // Register handler callbacks
    ws_server_.set_open_handler(bind(&WebSocketServer::on_open, this, std::placeholders::_1));
    ws_server_.set_close_handler(bind(&WebSocketServer::on_close, this, std::placeholders::_1));
}

void WebSocketServer::on_open(websocketpp::connection_hdl hdl) {
    std::lock_guard<std::mutex> guard(connection_mutex_);
    connections_.insert(hdl);
}

void WebSocketServer::on_close(websocketpp::connection_hdl hdl) {
    std::lock_guard<std::mutex> guard(connection_mutex_);
    connections_.erase(hdl);
}

void WebSocketServer::run(uint16_t port) {
    // Listen on specified port
    ws_server_.listen(port);

    // Start the server accept loop
    ws_server_.start_accept();

    // Start the ASIO io_service run loop
    ws_server_.run();
}

void WebSocketServer::broadcast(const std::string& message) {
    std::lock_guard<std::mutex> guard(connection_mutex_);
    for (auto& hdl : connections_) {
        ws_server_.send(hdl, message, websocketpp::frame::opcode::text);
    }
}
