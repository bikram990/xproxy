#ifndef SERVER_CONNECTION_H
#define SERVER_CONNECTION_H

#include "connection.h"
#include "counter.h"

class ServerConnection : public Connection, public Counter<ServerConnection> {
public:
    ServerConnection();
};

#endif // SERVER_CONNECTION_H
