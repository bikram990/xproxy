#include <boost/bind.hpp>
#include "client_connection_manager.h"
#include "connection.h"
#include "log.h"

void ClientConnectionManager::start(ConnectionPtr connection) {
    boost::asio::ip::tcp::socket& socket = connection->socket();
    std::string addr = socket.remote_endpoint().address().to_string();
    std::string port = std::to_string(socket.remote_endpoint().port());
    connections_.insert(std::pair<std::string, ConnectionPtr>(addr + ":" + port, connection));
    connection->start();
}

void ClientConnectionManager::stop(ConnectionPtr connection) {
    boost::asio::ip::tcp::socket& socket = connection->socket();
    std::string addr = socket.remote_endpoint().address().to_string();
    std::string port = std::to_string(socket.remote_endpoint().port());
    connections_.erase(addr + ":" + port);
}

ConnectionPtr ClientConnectionManager::find(const std::string& host, short port) {
    std::string key = host + ":" + std::to_string(port);
    auto it = connections_.find(key);
    if(it != connections_.end())
        return it->second;
    return ConnectionPtr();
}

void ClientConnectionManager::StopAll() {
    std::for_each(connections_.begin(), connections_.end(),
                  boost::bind(&Connection::stop,
                              boost::bind(&container_type::value_type::second, _1)));
}
