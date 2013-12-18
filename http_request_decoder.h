#ifndef HTTP_REQUEST_DECODER_H
#define HTTP_REQUEST_DECODER_H

#include "decoder.h"

class HttpChunk;
class HttpHeaders;
class HttpRequestInitial;
class HttpObject;

class HttpRequestDecoder : public Decoder {
public:
    HttpRequestDecoder();

    virtual ~HttpRequestDecoder();

    virtual DecodeResult decode(boost::asio::streambuf& buffer, HttpObject **object);

    virtual void reset() {
        result_ = kIncomplete;
        state_ = kRequestStart;
        if(temp_object_) {
            delete temp_object_;
            temp_object_ = nullptr;
        }
        body_length_ = 0;
    }

private:
    enum DecodeState {
        kRequestStart, kMethod, kUri, kProtocolH, kProtocolT1, kProtocolT2, kProtocolP,
        kSlash, kMajorVersionStart, kMajorVersion, kMinorVersionStart, kMinorVersion,
        kNewLineHeader, kHeaderStart, kNewLineBody, kHeaderLWS, // linear white space
        kHeaderName, kNewLineHeaderContinue, kHeaderValue, kHeaderValueSpaceBefore,
        kHeadersDone
    };

    void DecodeInitialLine(boost::asio::streambuf& buffer, HttpRequestInitial *initial);
    void DecodeHeaders(boost::asio::streambuf& buffer, HttpHeaders *headers);
    void DecodeBody(boost::asio::streambuf& buffer, HttpChunk *chunk);

    bool ischar(int c);
    bool istspecial(int c);

    DecodeResult result_;
    DecodeState state_;

    HttpObject *temp_object_;
    std::size_t body_length_;
};

#endif // HTTP_REQUEST_DECODER_H
