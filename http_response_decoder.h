#ifndef HTTP_RESPONSE_DECODER_H
#define HTTP_RESPONSE_DECODER_H

#include "decoder.h"

class HttpResponseInitial;
class HttpChunk;
class HttpHeaders;
class HttpObject;

class HttpResponseDecoder : public Decoder {
public:
    HttpResponseDecoder();

    virtual ~HttpResponseDecoder();

    virtual DecodeResult decode(boost::asio::streambuf& buffer, HttpObject **object);

private:
    enum DecodeState {
        kResponseStart, kProtocolH, kProtocolT1, kProtocolT2, kProtocolP,
        kSlash, kMajorVersionStart, kMajorVersion, kMinorVersionStart, kMinorVersion,
        kStatusCodeStart, kStatusCode, kStatusMessageStart, kStatusMessage,
        kNewLineHeader, kHeaderStart, kNewLineBody, kHeaderLWS, // linear white space
        kHeaderName, kNewLineHeaderContinue, kHeaderValue, kHeaderValueSpaceBefore,
        kHeadersDone
    };

    void DecodeInitialLine(boost::asio::streambuf& buffer, HttpResponseInitial *initial);
    void DecodeHeaders(boost::asio::streambuf& buffer, HttpHeaders *headers);
    void DecodeBody(boost::asio::streambuf& buffer, HttpChunk *chunk);

    DecodeResult result_;
    DecodeState state_;

    HttpObject *temp_object_;
    std::size_t body_length_;
    bool chunked_;
};

#endif // HTTP_RESPONSE_DECODER_H
