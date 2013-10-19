#include <boost/bind.hpp>
#include "http_direct_handler.h"
#include "http_proxy_session.h"

HttpDirectHandler::HttpDirectHandler(HttpProxySession& session,
                                     HttpRequestPtr request)
    : session_(session), local_socket_(session.LocalSocket()),
      request_(request), client_(session.service(), *request) {
    TRACE_THIS_PTR;
}

void HttpDirectHandler::HandleRequest() {
    XTRACE << "New request received, host: " << request_->host()
           << ", port: " << request_->port();
    client_.AsyncSendRequest(boost::bind(&HttpDirectHandler::OnResponseReceived,
                                         boost::static_pointer_cast<HttpDirectHandler>(shared_from_this()), _1, _2));
    XTRACE << "Request is sent asynchronously.";
}

void HttpDirectHandler::OnResponseReceived(const boost::system::error_code& e, HttpResponse *response) {
    if(e && e != boost::asio::error::eof) {
        XWARN << "Failed to send request, message: " << e.message();
        session_.Terminate();
        return;
    }

    XTRACE << "Response is back, status line: " << response->status_line();

    boost::asio::async_write(local_socket_, response->OutboundBuffer(),
                             boost::bind(&HttpDirectHandler::OnLocalDataSent,
                                         boost::static_pointer_cast<HttpDirectHandler>(shared_from_this()),
                                         boost::asio::placeholders::error));
}

void HttpDirectHandler::OnLocalDataSent(const boost::system::error_code& e) {
    if(e) {
        XWARN << "Failed to write response to local socket, message: " << e.message();
        session_.Terminate();
        return;
    }

    XTRACE << "Content written to local socket.";

    // TODO handle the persistent connection here

    if(!e) {
        boost::system::error_code ec;
        local_socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
    }

    if(e != boost::asio::error::operation_aborted) {
        session_.Terminate();
    }
}
