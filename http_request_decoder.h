#ifndef HTTP_REQUEST_DECODER_H
#define HTTP_REQUEST_DECODER_H

#include "decoder.h"

class HttpChunk;
class HttpHeaders;
class HttpObject;

class HttpRequestDecoder : public Decoder {
public:
    HttpRequestDecoder()
        : result_(kIncomplete), state_(kRequestStart),
          temp_object_(nullptr), body_length_(0) {}

    virtual ~HttpRequestDecoder() {
        if(temp_object_)
            delete temp_object_;
    }

    virtual DecodeResult decode(boost::asio::streambuf& buffer, HttpObject **object);

private:
    enum DecodeState {
        kRequestStart, kMethod, kUri, kProtocolH, kProtocolT1, kProtocolT2, kProtocolP,
        kSlash, kMajorVersionStart, kMajorVersion, kMinorVersionStart, kMinorVersion,
        kNewLineHeader, kHeaderStart, kNewLineBody, kHeaderLWS, // linear white space
        kHeaderName, kNewLineHeaderContinue, kHeaderValue, kHeaderValueSpaceBefore,
        kHeadersDone
    };

    void DecodeInitialLine(boost::asio::stream& buffer, HttpRequestInitial *initial);
    void DecodeHeaders(boost::asio::stream& buffer, HttpHeaders *headers);
    void DecodeBody(boost::asio::stream& buffer, HttpChunk *chunk);

    DecodeResult consume(char c);
    bool HttpRequestDecoder::ischar(int c);
    bool HttpRequestDecoder::istspecial(int c);

    DecodeResult result_;
    DecodeState state_;

    HttpObject *temp_object_;
    std::size_t body_length_;
};

#endif // HTTP_REQUEST_DECODER_H
