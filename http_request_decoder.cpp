#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include "byte_buffer.h"
#include "http_chunk.h"
#include "http_headers.h"
#include "http_request_decoder.h"
#include "http_request_initial.h"
#include "log.h"

#define ERR  Decoder::kFailure    // decode failed       (ERROR)
#define CTN  Decoder::kIncomplete // want more data      (CONTINUE)
#define DONE Decoder::kComplete   // object constructed  (DONE)
#define END  Decoder::kFinished   // the end of decoding (END)

HttpRequestDecoder::HttpRequestDecoder()
    : result_(kIncomplete), state_(kRequestStart),
      temp_object_(nullptr), body_length_(0) {}

HttpRequestDecoder::~HttpRequestDecoder() {
    if(temp_object_)
        delete temp_object_;
}

Decoder::DecodeResult HttpRequestDecoder::decode(boost::asio::streambuf& buffer, HttpObject **object) {
    if(!object) {
        XERROR << "Null pointer.";
        return kFailure;
    }

    if(result_ == kFinished || result_ == kFailure) {
        XERROR << "The request cannot accept more data, it is either finished or bad request.";
        return result_;
    }

    if(buffer.size() <= 0) {
        XERROR << "The stream is empty.";
        return kFailure;
    }

    if(result_ == kIncomplete) {
        if(!temp_object_) {
            XERROR << "Invalid object pointer.";
            return kFailure;
        }
        switch(temp_object_->type()) {
        case HttpObject::kHttpRequestInitial:
            DecodeInitialLine(buffer, reinterpret_cast<HttpRequestInitial*>(temp_object_));
            if(result_ == kComplete) {
                *object = temp_object_;
                temp_object_ = nullptr;
            }
            return result_;
        case HttpObject::kHttpHeaders: {
            HttpHeaders *headers = reinterpret_cast<HttpHeaders*>(temp_object_);
            DecodeHeaders(buffer, headers);
            if(result_ == kComplete) {
                *object = temp_object_;
                temp_object_ = nullptr;

                std::string content_length;
                if(headers->find("Content-Length", content_length)) { // TODO hard code
                    body_length_ = boost::lexical_cast<std::size_t>(content_length);
                } else {
                    result_ = kFinished; // there is no body
                }
            }
            return result_;
        }
        case HttpObject::kHttpChunk:
            DecodeBody(buffer, reinterpret_cast<HttpChunk*>(temp_object_));
            if(result_ == kFinished) {
                *object = temp_object_;
                temp_object_ = nullptr;
            }
            return result_;
        default:
            XERROR << "Invalid object type.";
            return kFailure;
        }
    }

    switch(state_) {
    case kRequestStart: {
        HttpRequestInitial *initial = new HttpRequestInitial();
        DecodeInitialLine(buffer, initial);
        if(result_ == kIncomplete) {
            temp_object_ = initial;
        }
        return result_;
    }
    case kHeaderStart: {
        HttpHeaders *headers = new HttpHeaders();
        DecodeHeaders(buffer, headers);
        if(result_ == kIncomplete) {
            temp_object_ = headers;
        } else if(result_ == kComplete) {
            std::string content_length;
            if(headers->find("Content-Length", content_length)) { // TODO hard code
                body_length_ = boost::lexical_cast<std::size_t>(content_length);
            } else {
                result_ = kFinished;
            }
        }
        return result_;
    }
    case kHeadersDone: {
        HttpChunk *chunk = new HttpChunk();
        DecodeBody(buffer, chunk);
        if(result_ == kIncomplete) {
            temp_object_ = chunk;
        }
        return result_;
    }
    default:
        XERROR << "Invalid state.";
        return kFailure;
    }

   // std::string host;
   // if(!request_->FindHeader("Host", host)) { // TODO hard code
   //     XDEBUG << "Host header not found, read host from the URI: " << uri_;
   //     std::string http("http://");
   //     if(uri_.compare(0, http.length(), http) != 0) {
   //         XERROR << "No valid host found.";
   //         return kFailure;
   //     }
   //     std::string::size_type end = uri_.find('/', http.length());
   //     if(end == std::string::npos) {
   //         XERROR << "No valid host found.";
   //         return kBadRequest;
   //     }
   //     host = uri_.substr(http.length(), end - http.length());
   //     XDEBUG << "Host found in uri: " << host;
   // }

   // std::string::size_type sep = host.find(':');
   // if(sep != std::string::npos) {
   //     host_ = host.substr(0, sep);
   //     port_ = boost::lexical_cast<short>(host.substr(sep + 1));
   // } else {
   //     host_ = host;

   //     if(method_ == "CONNECT") {
   //         std::string::size_type sep = uri_.find(':');
   //         if(sep != std::string::npos)
   //             port_ = boost::lexical_cast<short>(uri_.substr(sep + 1));
   //         else
   //             port_ = 443;
   //     } else {
   //         port_ = 80;
   //     }
   // }

   // CanonicalizeUri();

   // return result_;
}

inline void HttpRequestDecoder::DecodeInitialLine(boost::asio::streambuf& buffer, HttpRequestInitial *initial) {
    std::istream s(&buffer);
    s >> std::noskipws;

    char current_byte = 0;
    while(s >> current_byte) {
        switch(state_) {
        case kRequestStart:
            if(!std::isalpha(current_byte)) {
                result_ = ERR;
                break;
            }
            state_ = kMethod;
            initial->method().push_back(current_byte);
            result_ = CTN;
            break;
        case kMethod:
            if(current_byte == ' ') {
                state_ = kUri;
                result_ = CTN;
                break;
            }
            if(!std::isalpha(current_byte)) {
                result_ = ERR;
                break;
            }
            initial->method().push_back(current_byte);
            result_ = CTN;
            break;
        case kUri:
            if(current_byte == ' ') {
                state_ = kProtocolH;
                result_ = CTN;
                break;
            }
            if(std::iscntrl(current_byte)) {
                result_ = ERR;
                break;
            }
            initial->uri().push_back(current_byte);
            result_ = CTN;
            break;
        case kProtocolH:
            if(current_byte != 'H') {
                result_ = ERR;
                break;
            }
            state_ = kProtocolT1;
            result_ = CTN;
            break;
        case kProtocolT1:
            if(current_byte != 'T') {
                result_ = ERR;
                break;
            }
            state_ = kProtocolT2;
            result_ = CTN;
            break;
        case kProtocolT2:
            if(current_byte != 'T') {
                result_ = ERR;
                break;
            }
            state_ = kProtocolP;
            result_ = CTN;
            break;
        case kProtocolP:
            if(current_byte != 'P') {
                result_ = ERR;
                break;
            }
            state_ = kSlash;
            result_ = CTN;
            break;
        case kSlash:
            if(current_byte != '/') {
                result_ = ERR;
                break;
            }
            initial->SetMajorVersion(0);
            initial->SetMinorVersion(0);
            state_ = kMajorVersionStart;
            result_ = CTN;
            break;
        case kMajorVersionStart:
            if(!std::isdigit(current_byte)) {
                result_ = ERR;
                break;
            }
            initial->SetMajorVersion(initial->GetMajorVersion() * 10 + current_byte - '0');
            state_ = kMajorVersion;
            result_ = CTN;
            break;
        case kMajorVersion:
            if(current_byte == '.') {
                state_ = kMinorVersionStart;
                result_ = CTN;
                break;
            }
            if(!std::isdigit(current_byte)) {
                result_ = ERR;
                break;
            }
            initial->SetMajorVersion(initial->GetMajorVersion() * 10 + current_byte - '0');
            result_ = CTN;
            break;
        case kMinorVersionStart:
            if(!std::isdigit(current_byte)) {
                result_ = ERR;
                break;
            }
            initial->SetMinorVersion(initial->GetMinorVersion() * 10 + current_byte - '0');
            state_ = kMinorVersion;
            result_ = CTN;
            break;
        case kMinorVersion:
            if(current_byte == '\r') {
                state_ = kNewLineHeader;
                result_ = CTN;
                break;
            }
            if(!std::isdigit(current_byte)) {
                result_ = ERR;
                break;
            }
            initial->SetMinorVersion(initial->GetMinorVersion() * 10 + current_byte - '0');
            result_ = CTN;
            break;
        case kNewLineHeader:
            if(current_byte != '\n') {
                result_ = ERR;
                break;
            }
            state_ = kHeaderStart;
            result_ = DONE;
            break;
        default:
            result_ = ERR;
        }

        if(result_ == kFailure || result_ == kComplete)
            break;
    }
}

inline void HttpRequestDecoder::DecodeHeaders(boost::asio::streambuf& buffer, HttpHeaders *headers) {
    std::istream s(&buffer);
    s >> std::noskipws;

    char current_byte = 0;
    while(s >> current_byte) {
        switch(state_) {
        case kHeaderStart:
            if(current_byte == '\r') {
                state_ = kNewLineBody;
                result_ = CTN;
            }
            if(!headers->empty() && (current_byte == ' ' || current_byte == '\t')) {
                state_ = kHeaderLWS;
                result_ = CTN;
            }
            if(!ischar(current_byte) || std::iscntrl(current_byte) || istspecial(current_byte))
                result_ = ERR;
            //        if(!headers.empty()) {
            //            if(headers.back().name == "Host") {
            //                std::string& target = headers_.back().value;
            //                std::string::size_type seperator = target.find(':');
            //                if(seperator != std::string::npos) {
            //                    host_ = target.substr(0, seperator);
            //                    port_ = boost::lexical_cast<short>(target.substr(seperator + 1));
            //                } else {
            //                    host_ = target;
            //                    port_ = 80;
            //                }
            //            }
            //        }
            headers->PushBack(HttpHeader());
            headers->back().name.push_back(current_byte);
            state_ = kHeaderName;
            result_ = CTN;
            break;
        case kHeaderLWS:
            if(current_byte == '\r') {
                state_ = kNewLineHeaderContinue;
                result_ = CTN;
            }
            if(current_byte == ' ' || current_byte == '\t')
                result_ = CTN;
            if(std::iscntrl(current_byte))
                result_ = ERR;
            state_ = kHeaderValue;
            headers->back().value.push_back(current_byte);
            result_ = CTN;
            break;
        case kHeaderName:
            if(current_byte == ':') {
                state_ = kHeaderValueSpaceBefore;
                result_ = CTN;
            }
            if(!ischar(current_byte) || std::iscntrl(current_byte) || istspecial(current_byte))
                result_ = ERR;
            headers->back().name.push_back(current_byte);
            result_ = CTN;
            break;
        case kHeaderValueSpaceBefore:
            if(current_byte != ' ')
                result_ = ERR;
            state_ = kHeaderValue;
            result_ = CTN;
            break;
        case kHeaderValue:
            if(current_byte == '\r') {
                state_ = kNewLineHeaderContinue;
                result_ = CTN;
            }
            if(std::iscntrl(current_byte))
                result_ = ERR;
            headers->back().value.push_back(current_byte);
            result_ = CTN;
            break;
        case kNewLineHeaderContinue:
            if(current_byte != '\n')
                result_ = ERR;
            state_ = kHeaderStart;
            result_ = CTN;
            break;
        case kNewLineBody:
            if(current_byte != '\n')
                result_ = ERR;
            state_ = kHeadersDone;
            result_ = DONE;
            break;
        default:
            result_ = ERR;
        }

        if(result_ == kFailure || result_ == kComplete)
            break;
    }
}

inline void HttpRequestDecoder::DecodeBody(boost::asio::streambuf& buffer, HttpChunk *chunk) {
    /***********************************************************************
     * Here why dont we make a HttpChunk and push it into the body?
     * => As http request always has small body, so we wait until all the
     *    body data arrive, and then make only one HttpChunk.
     ***********************************************************************/
    if(body_length_ > buffer.size()) {
        result_ = kIncomplete;
        return;
    }

    *chunk << buffer; // TODO here we should only consume body_length_ bytes

    chunk->SetLast(true); // it will always be the last chunk

    result_ = kFinished;
}

inline bool HttpRequestDecoder::ischar(int c) {
    return c >= 0 && c <= 127;
}

inline bool HttpRequestDecoder::istspecial(int c) {
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

#undef ERR
#undef CTN
#undef DONE
#undef END
