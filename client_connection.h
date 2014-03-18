#ifndef CLIENT_CONNECTION_H
#define CLIENT_CONNECTION_H

#include "connection.h"
#include "counter.h"

class ClientConnection : public Connection, public Counter<ClientConnection> {
public:
    ClientConnection();
};

#endif // CLIENT_CONNECTION_H
