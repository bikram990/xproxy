#include <cctype>
#include <sstream>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include "log.h"
#include "http_request.h"

HttpRequest::HttpRequest() : state_(kRequestStart), port_(80), body_length_(0) {
    TRACE_THIS_PTR;
}

HttpRequest::~HttpRequest() {
    TRACE_THIS_PTR;
}

HttpRequest::BuildResult HttpRequest::BuildRequest(char *buffer, std::size_t length) {
    // TODO should we care if there is null character in buffer?
    std::stringstream req(buffer);
    std::string line;

    if(!std::getline(req, line)) {
        XDEBUG << "This request is incomplete.";
        return kNotComplete;
    }

    if(ConsumeInitialLine(line) == kBadRequest) {
        XERROR << "Invalid initial line: " << line;
        return kBadRequest;
    }

    BuildResult ret = kNotComplete;
    while(std::getline(req, line)) {
        if(line == "\r") { // no more headers, so all headers are parsed
            ret = kComplete;
            break;
        }
        BuildResult r = ConsumeHeaderLine(line);
        if(r == kBadRequest) {
            XERROR << "Invalid header line: " << line;
            return kBadRequest;
        } else if(r == kNotComplete) {
            XDEBUG << "The header line is incomplete: " << line;
            return kNotComplete;
        }
    }
    if(ret = kNotComplete) {// indicate not all headers are received
        XDEBUG << "Headers are incomplete.";
        return kNotComplete;
    }

    struct ContentLengthFinder {
        bool operator()(const HttpHeader& header) {
            return header.name == "Content-Length";
        }
    };

    if(method_ == "POST") {
        std::vector<HttpHeader>::iterator it = std::find_if(headers_.begin(),
                                                            headers_.end(),
                                                            ContentLengthFinder());
        if(it != headers_.end())
            body_length_ = boost::lexical_cast<std::size_t>(it->value);
        if(body_length_ > 0) {
            if(!std::getline(req, body_) || body_.length() < body_length_) {
                XDEBUG << "Incomplete body.";
                return kNotComplete;
            }
        }
    }

    return kComplete;
}

HttpRequest::BuildResult HttpRequest::ConsumeInitialLine(const std::string& line) {
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

HttpRequest::BuildResult HttpRequest::ConsumeHeaderLine(const std::string &line) {
    if(line[line.length() - 1] != '\r')
        return kNotComplete; // the last result of getline() may return incomplete header line, so we do this check here

    std::string::size_type sep_idx = line.find(": ");
    if(sep_idx == std::string::npos)
        return kBadRequest;

    std::string name = line.substr(0, sep_idx);
    std::string value = line.substr(sep_idx + 2, line.length() - 1 - name.length() - 2); // remove the last \r character
    headers_.push_back(HttpHeader(name, value));

    return kComplete;
}

/*
 * This function is almost copied from the URL below, I modified a little.
 * http://www.boost.org/doc/libs/1_54_0/doc/html/boost_asio/example/cpp03/http/server/request_parser.cpp
 */
HttpRequest::BuildResult HttpRequest::consume(char current_byte) {
#define ERR  HttpRequest::kBadRequest  // error
#define CTN  HttpRequest::kNotComplete // continue
#define DONE HttpRequest::kComplete    // complete

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
        if(current_byte == '\r') {
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
        if(!ischar(current_byte)
           || std::iscntrl(current_byte)
           || istspecial(current_byte))
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
