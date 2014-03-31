#ifndef CLIENT_CONNECTION_H
#define CLIENT_CONNECTION_H

#include <mutex>
#include "connection.h"
#include "counter.h"

class ClientConnection : public Connection, public Counter<ClientConnection> {
public:
    ClientConnection(std::shared_ptr<Session> session);
    virtual ~ClientConnection() = default;

    virtual void OnBodyComplete();

    virtual void init();

    virtual void OnMessageExchangeComplete();

    void WriteSSLReply();

    std::mutex& OutBufferLock() { return lock_; }

private:
    enum {
        kSocketTimeout = 60,
        kBufferSize = 2048
    };

    void ParseRemotePeer(const std::shared_ptr<Session>& session);

    virtual void connect() {} // do nothing here

    virtual void OnRead(const boost::system::error_code& e, std::size_t);

    virtual void OnWritten(const boost::system::error_code& e, std::size_t length);

    virtual void OnTimeout(const boost::system::error_code& e);

    void OnSSL(const boost::system::error_code& e, std::size_t length);

private:
    std::mutex lock_;
};

#endif // CLIENT_CONNECTION_H
