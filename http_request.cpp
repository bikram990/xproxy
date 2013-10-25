#include <cctype>
#include <sstream>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include "http_request.h"

HttpRequest::State HttpRequest::BuildRequest(char *buffer, std::size_t length,
                                             HttpRequest& request) {
    if(request.state_ == kComplete) {
        XWARN << "Overwritting a valid request.";
        request.headers_.clear();
        request.state_ = kEmptyRequest;
    }

    // TODO should we care if there is null character in buffer?
    std::stringstream req(buffer);
    std::string line;

    if(!buffer || length <= 0) {
        XERROR << "The buffer is invalid: null pointer or zero length.";
        return kBadRequest;
    }

    if(!std::getline(req, line)) {
        XDEBUG << "This request is incomplete.";
        return kIncomplete;
    }

    if(request.ConsumeInitialLine(line) == kBadRequest) {
        XERROR << "Invalid initial line: " << line;
        return kBadRequest;
    }

    State ret = kIncomplete;
    while(std::getline(req, line)) {
        if(line == "\r") { // no more headers, so all headers are parsed
            ret = kComplete;
            break;
        }
        State r = request.ConsumeHeaderLine(line);
        if(r == kBadRequest) {
            XERROR << "Invalid header line: " << line;
            return kBadRequest;
        } else if(r == kIncomplete) {
            XDEBUG << "The header line is incomplete: " << line;
            return kIncomplete;
        }
    }
    if(ret == kIncomplete) {// indicate not all headers are received
        XDEBUG << "Headers are incomplete.";
        return kIncomplete;
    }

    if(request.method_ == "POST") {
        const std::string& len = request.FindHeader("Content-Length");
        if(!len.empty())
            request.body_length(boost::lexical_cast<std::size_t>(len));
        if(request.body_length_ > 0) {
            // TODO enhance the logic here
            std::string body;
            if(!std::getline(req, body)) {
                XERROR << "Failed to read body";
                return kBadRequest;
            }
            if(body.length() < request.body_length_) {
                XDEBUG << "Incomplete body.";
                return kIncomplete;
            }
            request.body(body.begin(), body.end());
        }
    }

    std::string host = request.FindHeader("Host");
    if(host.empty()) {
        // if host is empty, we would find host in uri
        XDEBUG << "Host header not found, read host from the URI: " << request.uri_;
        std::string http("http://");
        if(request.uri_.compare(0, http.length(), http) != 0) {
            XERROR << "No valid host found.";
            return kBadRequest;
        }
        std::string::size_type end = request.uri_.find('/', http.length());
        if(end == std::string::npos) {
            XERROR << "No valid host found.";
            return kBadRequest;
        }
        host = request.uri_.substr(http.length(), end - http.length());
        XDEBUG << "Host found in uri: " << host;
    }

    std::string::size_type sep = host.find(':');
    if(sep != std::string::npos) {
        request.host_ = host.substr(0, sep);
        request.port_ = boost::lexical_cast<short>(host.substr(sep + 1));
    } else {
        request.host_ = host;

        if(request.method_ == "CONNECT") {
            std::string::size_type sep = request.uri_.find(':');
            if(sep != std::string::npos)
                request.port_ = boost::lexical_cast<short>(request.uri_.substr(sep + 1));
            else
                request.port_ = 443;
        } else {
            request.port_ = 80;
        }
    }

    CanonicalizeUri(request.uri_);

    request.state_ = kComplete;
    return kComplete;
}

void HttpRequest::CanonicalizeUri(std::string& uri) {
    XDEBUG << "Canonicalize URI: " << uri;

    if(uri[0] == '/') { // already canonicalized
        XDEBUG << "URI is already canonicalized: " << uri;
        return;
    }

    std::string http("http://");
    std::string::size_type end = std::string::npos;
    if(uri.compare(0, http.length(), http) != 0)
        end = uri.find('/');
    else
        end = uri.find('/', http.length());

    if(end == std::string::npos) {
        XDEBUG << "No host end / found, consider as the root: " << uri;
        uri = '/';
        return;
    }

    uri.erase(0, end);
    XDEBUG << "URI canonicalized: " << uri;
}

boost::asio::streambuf& HttpRequest::OutboundBuffer() {
    if(buffer_built_)
        return raw_buffer_;

    std::ostream buf(&raw_buffer_);

    buf << method_ << ' ' << uri_ << ' '
        << "HTTP/" << major_version_ << '.' << minor_version_ << "\r\n";

    for(std::vector<HttpHeader>::iterator it = headers_.begin();
        it != headers_.end(); ++it)
        buf << it->name << ": " << it->value << "\r\n";

    buf << "\r\n";

    if(body_length_ > 0)
        buf << body_;

    buffer_built_ = true;
    return raw_buffer_;
}

HttpRequest::State HttpRequest::ConsumeInitialLine(const std::string& line) {
    std::stringstream ss(line);

    if(!std::getline(ss, method_, ' '))
        return kBadRequest;

    if(!std::getline(ss, uri_, ' '))
        return kBadRequest;

    std::string item;

    if(!std::getline(ss, item, '/')) // the "HTTP" string, do nothing here
        return kBadRequest;

    if(!std::getline(ss, item, '.')) // http major version
        return kBadRequest;
    major_version_ = boost::lexical_cast<int>(item);

    if(!std::getline(ss, item, '\r')) // remember there is a \r character at the end of line
        return kBadRequest;
    minor_version_ = boost::lexical_cast<int>(item);

    return kComplete;
}

HttpRequest::State HttpRequest::ConsumeHeaderLine(const std::string& line) {
    if(line[line.length() - 1] != '\r')
        return kIncomplete; // the last result of getline() may return incomplete header line, so we do this check here

    std::string::size_type sep_idx = line.find(": ");
    if(sep_idx == std::string::npos)
        return kBadRequest;

    std::string name = line.substr(0, sep_idx);
    std::string value = line.substr(sep_idx + 2, line.length() - 1 - name.length() - 2); // remove the last \r character
    AddHeader(name, value);

    return kComplete;
}

const std::string& HttpRequest::FindHeader(const std::string& name) {
    std::vector<HttpHeader>::iterator it = std::find_if(headers_.begin(),
                                                        headers_.end(),
                                                        HeaderFinder(name));
    return it->value;
}
