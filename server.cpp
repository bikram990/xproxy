#include <cstdlib>
#include <csignal>
#include <string>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/asio/signal_set.hpp>
#include "log.hpp"

using boost::asio::ip::tcp;

#define LOG_INFO(info) std::cout << "[INFO]  " << info << std::endl
#define LOG_ERROR(err) std::cout << "[ERROR] " << err  << std::endl

static std::string resp_content("Proxy is at work.");
static std::string resp_status_line("HTTP/1.1 200 OK\r\nContent-Length:17\r\n\r\n");
static std::string resp("HTTP/1.1 200 OK\r\nContent-Length:17\r\n\r\nProxy is at work.");

class session
{
public:
    session(boost::asio::io_service& io_service)
            : socket_(io_service)
    {
    }

    tcp::socket& socket()
    {
        return socket_;
    }

    void start()
    {
        socket_.async_read_some(boost::asio::buffer(data_, max_length),
                                boost::bind(&session::handle_read, this,
                                            boost::asio::placeholders::error,
                                            boost::asio::placeholders::bytes_transferred));
    }

private:
    void handle_read(const boost::system::error_code& error,
                     size_t bytes_transferred)
    {
        if (!error)
        {
            LOG_INFO(data_);
            LOG_INFO("writing back status 200.");
            boost::asio::async_write(socket_,
                                     boost::asio::buffer(resp),
                                     boost::bind(&session::handle_write, this,
                                                 boost::asio::placeholders::error));
        }
        else
        {
            LOG_ERROR("failed to read content from socket.");
            delete this;
        }
    }

    void handle_write(const boost::system::error_code& error)
    {
        if (!error)
        {
            LOG_INFO("data written.");
            socket_.async_read_some(boost::asio::buffer(data_, max_length),
                                    boost::bind(&session::handle_read, this,
                                                boost::asio::placeholders::error,
                                                boost::asio::placeholders::bytes_transferred));
        }
        else
        {
            LOG_ERROR("failed to write data.");
            delete this;
        }
    }

    tcp::socket socket_;
    enum { max_length = 1024 };
    char data_[max_length];
};

class server
{
public:
    server(boost::asio::io_service& io_service, short port)
            : io_service_(io_service),
              acceptor_(io_service, tcp::endpoint(tcp::v4(), port))
    {
        start_accept();
    }

private:
    void start_accept()
    {
        session* new_session = new session(io_service_);
        acceptor_.async_accept(new_session->socket(),
                               boost::bind(&server::handle_accept, this, new_session,
                                           boost::asio::placeholders::error));
    }

    void handle_accept(session* new_session,
                       const boost::system::error_code& error)
    {
        if (!error)
        {
            std::string addr = new_session->socket().remote_endpoint().address().to_string();
            unsigned port = new_session->socket().remote_endpoint().port();
            LOG_INFO(addr);
            LOG_INFO(port);
            new_session->start();
        }
        else
        {
            LOG_ERROR("error accept new connection");
            delete new_session;
        }

        start_accept();
    }

    boost::asio::io_service& io_service_;
    tcp::acceptor acceptor_;
};

int main(int argc, char* argv[]) {
    Log::set_debug_level(boost::log::trivial::debug);
    Log::debug("xProxy is starting...");
    try {
        boost::asio::io_service io_service;
        boost::asio::signal_set sigs(io_service, SIGINT, SIGTERM);
        sigs.async_wait(boost::bind(&boost::asio::io_service::stop, &io_service));

        server s(io_service, 7077);
        io_service.run();
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }
    Log::debug("xProxy is stopped.");
    return 0;
}
