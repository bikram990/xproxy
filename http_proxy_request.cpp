#include <cctype>
#include <boost/lexical_cast.hpp>
#include "http_proxy_request.h"

HttpProxyRequest::HttpProxyRequest() : state_(kRequestStart) {
}

HttpProxyRequest::~HttpProxyRequest() {
}

HttpProxyRequest::BuildResult HttpProxyRequest::BuildFromRaw(char *buffer, std::size_t length) {
    for(std::size_t i = 0; i < length; ++i) {
        std::ostream buf(&raw_buffer_);
        buf << *buffer;

        HttpProxyRequest::BuildResult result = consume(*buffer++);
        if(result == HttpProxyRequest::kBuildError)
            return result;
        if(result == HttpProxyRequest::kComplete)
            if(i < length - 1) // there is more content, for body
                body_ = buffer;
            return result;
    }
    return HttpProxyRequest::kNotComplete;
}

/*
 * This function is almost copied from the URL below, I modified a little.
 * http://www.boost.org/doc/libs/1_54_0/doc/html/boost_asio/example/cpp03/http/server/request_parser.cpp
 */
HttpProxyRequest::BuildResult HttpProxyRequest::consume(char current_byte) {
#define ERR  HttpProxyRequest::kBuildError  // error
#define CTN  HttpProxyRequest::kNotComplete // continue
#define DONE HttpProxyRequest::kComplete    // complete

    switch(state_) {
    case kRequestStart:
        if(!std::isalpha(current_byte))
            return ERR;
        state_ = kMethod;
        method_.push_back(current_byte);
        return CTN;
    case kMethod:
        if(current_byte == ' ') {
            state_ = kUri;
            return CTN;
        }
        if(!std::isalpha(current_byte))
            return ERR;
        method_.push_back(current_byte);
        return CTN;
    case kUri:
        if(current_byte == ' ') {
            state_ = kProtocolH;
            return CTN;
        }
        if(std::iscntrl(current_byte))
            return ERR;
        uri_.push_back(current_byte);
        return CTN;
    case kProtocolH:
        if(current_byte != 'H')
            return ERR;
        state_ = kProtocolT1;
        return CTN;
    case kProtocolT1:
        if(current_byte != 'T')
            return ERR;
        state_ = kProtocolT2;
        return CTN;
    case kProtocolT2:
        if(current_byte != 'T')
            return ERR;
        state_ = kProtocolP;
        return CTN;
    case kProtocolP:
        if(current_byte != 'P')
            return ERR;
        state_ = kSlash;
        return CTN;
    case kSlash:
        if(current_byte != '/')
            return ERR;
        major_version_ = 0;
        minor_version_ = 0;
        state_ = kMajorVersionStart;
        return CTN;
    case kMajorVersionStart:
        if(!std::isdigit(current_byte))
            return ERR;
        major_version_ = major_version_ * 10 + current_byte - '0';
        state_ = kMajorVersion;
        return CTN;
    case kMajorVersion:
        if(current_byte == '.') {
            state_ = kMinorVersionStart;
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
        state_ = kMinorVersion;
        return CTN;
    case kMinorVersion:
        if(current_byte = '\r') {
            state_ = kNewLineHeader;
            return CTN;
        }
        if(!std::isdigit(current_byte))
            return ERR;
        minor_version_ = minor_version_ * 10 + current_byte - '0';
        return CTN;
    case kNewLineHeader:
        if(current_byte != '\n')
            return ERR;
        state_ = kHeaderStart;
        return CTN;
    case kHeaderStart:
        if(current_byte == '\r') {
            state_ = kNewLineBody;
            return CTN;
        }
        if(!headers_.empty() && (current_byte == ' '
                                 || current_byte == '\t')) {
            state_ = kHeaderLWS;
            return CTN;
        }
        if(!std::isalnum(current_byte)) // TODO check here
            return CTN;
        if(!headers_.empty())
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
        headers_.push_back(Header());
        headers_.back().name.push_back(current_byte);
        state_ = kHeaderName;
        return CTN;
    case kHeaderLWS:
        if(current_byte == '\r') {
            state_ = kNewLineHeaderContinue;
            return CTN;
        }
        if(current_byte == ' ' || current_byte == '\t')
            return CTN;
        if(std::iscntrl(current_byte))
            return ERR;
        state_ = kHeaderValue;
        headers_.back().value.push_back(current_byte);
        return CTN;
    case kHeaderName:
        if(current_byte == ':') {
            state_ = kHeaderValueSpaceBefore;
            return CTN;
        }
        if(!std::isalnum(current_byte)) // TODO check here
            return ERR;
        headers_.back().name.push_back(current_byte);
        return CTN;
    case kHeaderValueSpaceBefore:
        if(current_byte != ' ')
            return ERR;
        state_ = kHeaderValue;
        return CTN;
    case kHeaderValue:
        if(current_byte == '\r') {
            state_ = kNewLineHeaderContinue;
            return CTN;
        }
        if(std::iscntrl(current_byte))
            return ERR;
        headers_.back().value.push_back(current_byte);
        return CTN;
    case kNewLineHeaderContinue:
        if(current_byte != '\n')
            return ERR;
        state_ = kHeaderStart;
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
