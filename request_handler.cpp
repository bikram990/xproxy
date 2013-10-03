#include "log.h"
#include "http_direct_handler.h"
#include "request_handler.h"

RequestHandler::RequestHandler() {
    TRACE_THIS_PTR;
}

RequestHandler::~RequestHandler() {
    TRACE_THIS_PTR;
}

RequestHandler *RequestHandler::CreateHandler(char *data, std::size_t size,
                                              HttpProxySession &session,
                                              boost::asio::io_service &service,
                                              boost::asio::ip::tcp::socket &local_socket,
                                              boost::asio::ip::tcp::socket &remote_socket) {
    if(!data)
        return NULL;//nullptr;
    if(data[0] == 'C' && data[1] == 'O' && data[2] == 'N')
        return NULL;//nullptr; // TODO this is https connection, implement it later
    // TODO may add socks handler here
    else
        return CreateHttpHandler(data, size, session, service, local_socket, remote_socket);
}

RequestHandler *RequestHandler::CreateHttpHandler(char *data, std::size_t size,
                                                  HttpProxySession &session,
                                                  boost::asio::io_service &service,
                                                  boost::asio::ip::tcp::socket &local_socket,
                                                  boost::asio::ip::tcp::socket &remote_socket) {
    XTRACE << "Dump data from local socket(size:" << size << "):\n"
           << "--------------------------------------------\n"
           << data
           << "\n--------------------------------------------";
    HttpRequestPtr request(boost::make_shared<HttpRequest>());
    ResultType result = HttpRequest::BuildRequest(data, size, *request);
    if(result == HttpRequest::kIncomplete) {
        XWARN << "Not a complete request, but currently partial request cannot be handled.";
        return NULL;
    }
    if(result == HttpRequest::kBadRequest) { // kNotComplete or kBadRequest
        XWARN << "Bad request: " << local_socket.remote_endpoint().address()
              << ":" << local_socket.remote_endpoint().port();
        return NULL;
    }
    // TODO add logic here after HttpProxyHandler is implemented
    return new HttpDirectHandler(session, service, local_socket, remote_socket, request);
}
