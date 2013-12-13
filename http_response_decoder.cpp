#include <boost/asio.hpp>
#include "byte_buffer.h"
#include "http_chunk.h"
#include "http_headers.h"
#include "http_response_decoder.h"
#include "http_response_initial.h"
#include "log.h"

#define ERR  Decoder::kFailure    // decode failed       (ERROR)
#define CTN  Decoder::kIncomplete // want more data      (CONTINUE)
#define DONE Decoder::kComplete   // object constructed  (DONE)
#define END  Decoder::kFinished   // the end of decoding (END)

HttpResponseDecoder::HttpResponseDecoder()
    : result_(kIncomplete), state_(kResponseStart) {}

Decoder::DecodeResult HttpResponseDecoder::decode(boost::asio::streambuf& buffer, HttpObject **object) {
    if(!object) {
        XERROR << "Null pointer.";
        return kFailure;
    }

    if(result_ == kFinished || result_ == kFailure) {
        XERROR << "The response cannot accept more data, it is either finished or bad request.";
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
        case HttpObject::kHttpResponseInitial:
            DecodeInitialLine(buffer, reinterpret_cast<HttpResponeInitial*>(temp_object_));
            if(result_ == kComplete) {
                *object = temp_object_;
                temp_object_ = nullptr;
            }
            return result_;
        case HttpObject::kHttpHeaders:
            HttpHeaders *headers = reinterpret_cast<HttpHeaders*>(temp_object_);
            DecodeHeaders(buffer, headers);
            if(result_ == kComplete) {
                *object = temp_object_;
                temp_object_ = nullptr;

                std::string transfer_encoding;
                std::string content_length;
                if(headers->find("Transfer-Encoding", transfer_encoding)) {
                    if(transfer_encoding == "chunked")
                        chunked_ = true;
                } else if(headers->find("Content-Length", content_length)) { // TODO hard code
                    if(chunked)
                        XWARN << "Both Transfer-Encoding and Content-Length headers are found, this should never happen.";
                    body_length_ = boost::lexical_cast<std::size_t>(content_length);
                } else {
                    result_ = kFinished; // there is no body
                }
            }
            return result_;
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
    case kResponseStart:
        HttpResponseInitial *initial = new HttpResponseInitial();
        DecodeInitialLine(buffer, initial);
        if(result_ == kIncomplete) {
            temp_object_ = initial;
        } else if(result_ = kComplete) {
            *object = initial;
        }
        return result_;
    case kHeaderStart:
        HttpHeaders *headers = new HttpHeaders();
        DecodeHeaders(buffer, headers);
        if(result_ == kIncomplete) {
            temp_object_ = headers;
        } else if(result_ == kComplete) {
            std::string transfer_encoding;
            std::string content_length;
            if(headers->find("Transfer-Encoding", transfer_encoding)) {
                if(transfer_encoding == "chunked")
                    chunked_ = true;
            } else if(headers->find("Content-Length", content_length)) { // TODO hard code
                if(chunked)
                    XWARN << "Both Transfer-Encoding and Content-Length headers are found, this should never happen.";
                body_length_ = boost::lexical_cast<std::size_t>(content_length);
            } else {
                result_ = kFinished; // there is no body
            }
            *object = headers;
        }
        return result_;
    case kHeadersDone:
        HttpChunk *chunk = new HttpChunk();
        DecodeBody(buffer, chunk);
        if(result_ == kIncomplete) {
            temp_object_ = chunk;
        } else if(result_ == kComplete || result_ == kFinished) {
            *object = chunk;
        }
        return result_;
    default:
        XERROR << "Invalid state.";
        return kFailure;
    }
}

inline void HttpResponseDecoder::DecodeInitialLine(boost::asio::streambuf& buffer, HttpResponseInitial *initial) {
    std::istream s(&buffer);
    s >> std::noskipws;

    char current_byte = 0;
    while(s >> current_byte) {
        switch(state_) {
        case kResponseStart:
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
                resutl_ = ERR;
                break;
            }
            state_ = kSlash;
            result_ = CTN;
            break;
        case kSlash:
            if(current_byte != '/') {
                resutl_ = ERR;
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
            if(current_byte == ' ') {
                state_ = kStatusCodeStart;
                initial->SetStatusCode(0);
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
        case kStatusCodeStart:
            if(!std::isdigit(current_byte)) {
                result_ = ERR;
                break;
            }
            initial->SetStatusCode(initial->GetStatusCode() * 10 + current_byte - '0');
            state_ = kStatusCode;
            result_ = CTN;
            break;
        case kStatusCode:
            if(current_byte == ' ') {
                state_ = kStatusMessageStart;
                result_ = CTN;
                break;
            }
            if(!std::isdigit(current_byte)) {
                result_ = ERR;
                break;
            }
            initial->SetStatusCode(initial->GetStatusCode() * 10 + current_byte - '0');
            result_ = CTN;
            break;
        case kStatusMessageStart:
            if(!std::isalpha(current_byte)) {
                result_ = ERR;
                break;
            }
            initial->StatusMessage().push_back(current_byte);
            state_ = kStatusMessage;
            result_ = CTN;
            break;
        case kStatusMessage:
            if(current_byte == '\r') {
                state_ = kNewLineHeader;
                result_ = CTN;
                break;
            }
            if(!std::isalpha(current_byte)) {
                result_ = ERR;
                break;
            }
            initial->StatusMessage().push_back(current_byte);
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
            break;
        }

        if(result_ == kFailure || result_ == kComplete)
            break;
    }
}

inline void HttpResponseDecoder::DecodeHeaders(boost::asio::streambuf& buffer, HttpHeaders *headers) {
    std::istream s(&buffer);
    s >> std::noskipws;

    char current_byte = 0;
    while(s >> current_byte) {
        switch(state_) {
        case kHeaderStart:
            if(current_byte == '\r') {
                state_ = kNewLineBody;
                result_ = CTN;
                break;
            }
            if(!headers.empty() && (current_byte == ' ' || current_byte == '\t')) {
                state_ = kHeaderLWS;
                result_ = CTN;
                break;
            }
            if(!ischar(current_byte) || std::iscntrl(current_byte) || istspecial(current_byte)) {
                result_ = ERR;
                break;
            }
            headers.PushBack(HttpHeader());
            headers.back().name.push_back(current_byte);
            state_ = kHeaderName;
            result_ = CTN;
            break;
        case kHeaderLWS:
            if(current_byte == '\r') {
                state_ = kNewLineHeaderContinue;
                result_ = CTN;
                break;
            }
            if(current_byte == ' ' || current_byte == '\t') {
                result_ = CTN;
                break;
            }
            if(std::iscntrl(current_byte)) {
                result_ = ERR;
                break;
            }
            state_ = kHeaderValue;
            headers.back().value.push_back(current_byte);
            result_ = CTN;
            break;
        case kHeaderName:
            if(current_byte == ':') {
                state_ = kHeaderValueSpaceBefore;
                result_ = CTN;
                break;
            }
            if(!ischar(current_byte) || std::iscntrl(current_byte) || istspecial(current_byte)) {
                result_ = ERR;
                break;
            }
            headers.back().name.push_back(current_byte);
            result_ = CTN;
            break;
        case kHeaderValueSpaceBefore:
            if(current_byte != ' ') {
                result_ = ERR;
                break;
            }
            state_ = kHeaderValue;
            result_ = CTN;
            break;
        case kHeaderValue:
            if(current_byte == '\r') {
                state_ = kNewLineHeaderContinue;
                result_ = CTN;
                break;
            }
            if(std::iscntrl(current_byte)) {
                result_ = ERR;
                break;
            }
            headers.back().value.push_back(current_byte);
            result_ = CTN;
            break;
        case kNewLineHeaderContinue:
            if(current_byte != '\n') {
                result_ = ERR;
                break;
            }
            state_ = kHeaderStart;
            result_ = CTN;
            break;
        case kNewLineBody:
            if(current_byte != '\n') {
                result_ = ERR;
                break;
            }
            state_ = kHeadersDone;
            result_ = DONE;
            break;
        default:
            result_ = ERR;
            break;
        }

        if(result_ == kFailure || result_ == kComplete)
            break;
    }
}

inline void HttpResponseDecoder::DecodeBody(boost::asio::streambuf &buffer, HttpChunk *chunk) {
    std::size_t size = buffer.size();

    *chunk << buffer; // TODO here we should only consume body_length_ bytes, but it does not matter much

    if(chunked_) {
        char *data = chunk->ByteContent()->data();
        if(*(data + chunk->ByteContent()->size() - 4) == '\r'
                && *(data + chunk->ByteContent()->size() - 3) == '\n'
                && *(data + chunk->ByteContent()->size() - 2) == '\r'
                && *(data + chunk->ByteContent()->size() - 1) == '\n') {
            chunk->SetLast(true);
            result_ = kFinished;
        } else {
            chunk->SetLast(false);
            result_ = kComplete;
        }
    } else {
        body_length_ -= size;
        if(body_length_ <= 0) { // do not need more data
            chunk->SetLast(true);
            result_ = kFinished;
        } else {
            chunk->SetLast(false);
            result_ = kComplete;
        }
    }
}

#undef ERR
#undef CTN
#undef DONE
#undef END
