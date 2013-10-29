#include <boost/lexical_cast.hpp>
#include "http_request.h"

HttpRequest::State HttpRequest::operator<<(boost::asio::streambuf& stream) {
    if(state_ != kIncomplete) {
        XERROR << "The request cannot accept more data, it is either completed or bad request.";
        return state_;
    }
    if(stream.size() <= 0) {
        XERROR << "The stream is empty.";
        state_ = kBadRequest;
        return state_;
    }

    std::istream s(&stream);
    s >> std::noskipws;

    char c = 0;
    while(s >> c) {
        state_ = consume(c);
        if(state_ == kBadRequest || state_ == kComplete)
            break;
    }

    if(state_ != kComplete)
        return state_;

    std::string content_length;
    if(FindHeader("Content-Length", content_length)) {
        std::size_t body_len = boost::lexical_cast<std::size_t>(content_length);
        XDEBUG << "The Content-Length header: " << body_len
               << ", current stream size: " << stream.size();
        if(body_len > stream.size()) {
            state_ = kIncomplete;
            return state_;
        }
        std::size_t copied = boost::asio::buffer_copy(body_.prepare(body_len), stream.data());
        body_.commit(copied);
        stream.consume(copied);
        XDEBUG << "The request body length is: " << body_.size();
    }

    std::string host;
    if(!FindHeader("Host", host)) {
        XDEBUG << "Host header not found, read host from the URI: " << uri_;
        std::string http("http://");
        if(uri_.compare(0, http.length(), http) != 0) {
            XERROR << "No valid host found.";
            return kBadRequest;
        }
        std::string::size_type end = uri_.find('/', http.length());
        if(end == std::string::npos) {
            XERROR << "No valid host found.";
            return kBadRequest;
        }
        host = uri_.substr(http.length(), end - http.length());
        XDEBUG << "Host found in uri: " << host;
    }

    std::string::size_type sep = host.find(':');
    if(sep != std::string::npos) {
        host_ = host.substr(0, sep);
        port_ = boost::lexical_cast<short>(host.substr(sep + 1));
    } else {
        host_ = host;

        if(method_ == "CONNECT") {
            std::string::size_type sep = uri_.find(':');
            if(sep != std::string::npos)
                port_ = boost::lexical_cast<short>(uri_.substr(sep + 1));
            else
                port_ = 443;
        } else {
            port_ = 80;
        }
    }

    CanonicalizeUri();

    return state_;
}

void HttpRequest::reset() {
    XDEBUG << "Resetting request...";

    if(buffer_built_) {
        raw_buffer_.consume(raw_buffer_.size());
        buffer_built_ = false;
    }

    state_ = kIncomplete;
    build_state_ = kRequestStart;

    host_ = "";
    port_ = 80;
    method_ = "";
    uri_ = "";
    major_version_ = 0;
    minor_version_ = 0;
    headers_.clear();
    if(body_.size() > 0)
        body_.consume(body_.size());
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

    if(body_.size() > 0) {
        std::size_t copied = boost::asio::buffer_copy(raw_buffer_.prepare(body_.size()), body_.data());
        raw_buffer_.commit(copied);
    }

    buffer_built_ = true;
    return raw_buffer_;
}

inline bool HttpRequest::FindHeader(const std::string& name, std::string& value) {
    std::vector<HttpHeader>::iterator it = std::find_if(headers_.begin(),
                                                        headers_.end(),
                                                        HeaderFinder(name));
    if(it == headers_.end())
        return false;
    value = it->value;
    return true;
}

inline void HttpRequest::CanonicalizeUri() {
    XDEBUG << "Canonicalize URI: " << uri_;

    if(uri_[0] == '/') { // already canonicalized
        XDEBUG << "URI is already canonicalized: " << uri_;
        return;
    }

    std::string http("http://");
    std::string::size_type end = std::string::npos;
    if(uri_.compare(0, http.length(), http) != 0)
        end = uri_.find('/');
    else
        end = uri_.find('/', http.length());

    if(end == std::string::npos) {
        XDEBUG << "No host end / found, consider as the root: " << uri_;
        uri_ = '/';
        return;
    }

    uri_.erase(0, end);
    XDEBUG << "URI canonicalized: " << uri_;
}

inline HttpRequest::State HttpRequest::consume(char current_byte) {
#define ERR HttpRequest::kBadRequest // bad request
#define CTN HttpRequest::kIncomplete // continue
#define DONE HttpRequest::kComplete  // complete

    switch(build_state_) {
    case kRequestStart:
        if(!std::isalpha(current_byte))
            return ERR;
        build_state_ = kMethod;
        method_.push_back(current_byte);
        return CTN;
    case kMethod:
        if(current_byte == ' ') {
            build_state_ = kUri;
            return CTN;
        }
        if(!std::isalpha(current_byte))
            return ERR;
        method_.push_back(current_byte);
        return CTN;
    case kUri:
        if(current_byte == ' ') {
            build_state_ = kProtocolH;
            return CTN;
        }
        if(std::iscntrl(current_byte))
            return ERR;
        uri_.push_back(current_byte);
        return CTN;
    case kProtocolH:
        if(current_byte != 'H')
            return ERR;
        build_state_ = kProtocolT1;
        return CTN;
    case kProtocolT1:
        if(current_byte != 'T')
            return ERR;
        build_state_ = kProtocolT2;
        return CTN;
    case kProtocolT2:
        if(current_byte != 'T')
            return ERR;
        build_state_ = kProtocolP;
        return CTN;
    case kProtocolP:
        if(current_byte != 'P')
            return ERR;
        build_state_ = kSlash;
        return CTN;
    case kSlash:
        if(current_byte != '/')
            return ERR;
        major_version_ = 0;
        minor_version_ = 0;
        build_state_ = kMajorVersionStart;
        return CTN;
    case kMajorVersionStart:
        if(!std::isdigit(current_byte))
            return ERR;
        major_version_ = major_version_ * 10 + current_byte - '0';
        build_state_ = kMajorVersion;
        return CTN;
    case kMajorVersion:
        if(current_byte == '.') {
            build_state_ = kMinorVersionStart;
            return CTN;
        }
        if(!std::isdigit(current_byte))
            return ERR;
        major_version_ = major_version_ * 10 + current_byte - '0';
        return CTN;
    case kMinorVersionStart:
        if(!std::isdigit(current_byte))
            return ERR;
        minor_version_ = minor_version_ * 10 + current_byte - '0';
        build_state_ = kMinorVersion;
        return CTN;
    case kMinorVersion:
        if(current_byte == '\r') {
            build_state_ = kNewLineHeader;
            return CTN;
        }
        if(!std::isdigit(current_byte))
            return ERR;
        minor_version_ = minor_version_ * 10 + current_byte - '0';
        return CTN;
    case kNewLineHeader:
        if(current_byte != '\n')
            return ERR;
        build_state_ = kHeaderStart;
        return CTN;
    case kHeaderStart:
        if(current_byte == '\r') {
            build_state_ = kNewLineBody;
            return CTN;
        }
        if(!headers_.empty() && (current_byte == ' '
                                 || current_byte == '\t')) {
            build_state_ = kHeaderLWS;
            return CTN;
        }
        if(!ischar(current_byte)
           || std::iscntrl(current_byte)
           || istspecial(current_byte))
            return ERR;
        if(!headers_.empty()) {
            if(headers_.back().name == "Host") {
                std::string& target = headers_.back().value;
                std::string::size_type seperator = target.find(':');
                if(seperator != std::string::npos) {
                    host_ = target.substr(0, seperator);
                    port_ = boost::lexical_cast<short>(target.substr(seperator + 1));
                } else {
                    host_ = target;
                    port_ = 80;
                }
            }
        }
        headers_.push_back(HttpHeader());
        headers_.back().name.push_back(current_byte);
        build_state_ = kHeaderName;
        return CTN;
    case kHeaderLWS:
        if(current_byte == '\r') {
            build_state_ = kNewLineHeaderContinue;
            return CTN;
        }
        if(current_byte == ' ' || current_byte == '\t')
            return CTN;
        if(std::iscntrl(current_byte))
            return ERR;
        build_state_ = kHeaderValue;
        headers_.back().value.push_back(current_byte);
        return CTN;
    case kHeaderName:
        if(current_byte == ':') {
            build_state_ = kHeaderValueSpaceBefore;
            return CTN;
        }
        if(!ischar(current_byte)
           || std::iscntrl(current_byte)
           || istspecial(current_byte))
            return ERR;
        headers_.back().name.push_back(current_byte);
        return CTN;
    case kHeaderValueSpaceBefore:
        if(current_byte != ' ')
            return ERR;
        build_state_ = kHeaderValue;
        return CTN;
    case kHeaderValue:
        if(current_byte == '\r') {
            build_state_ = kNewLineHeaderContinue;
            return CTN;
        }
        if(std::iscntrl(current_byte))
            return ERR;
        headers_.back().value.push_back(current_byte);
        return CTN;
    case kNewLineHeaderContinue:
        if(current_byte != '\n')
            return ERR;
        build_state_ = kHeaderStart;
        return CTN;
    case kNewLineBody:
        if(current_byte != '\n')
            return ERR;
        return DONE;
    default:
        return ERR;
    }

#undef ERR
#undef CTN
#undef DONE
}

inline bool HttpRequest::ischar(int c) {
    return c >= 0 && c <= 127;
}

inline bool HttpRequest::istspecial(int c) {
    switch(c) {
    case '(': case ')': case '<': case '>': case '@':
    case ',': case ';': case ':': case '\\': case '"':
    case '/': case '[': case ']': case '?': case '=':
    case '{': case '}': case ' ': case '\t':
        return true;
    default:
        return false;
    }
}
