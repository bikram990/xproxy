#ifndef HTTP_RESPONSE_DECODER_H
#define HTTP_RESPONSE_DECODER_H

#include "decoder.h"

class HttpChunk;
class HttpHeaders;
class HttpResponseInitial;
class HttpObject;

class HttpResponseDecoder : public Decoder {
public:
    HttpResponseDecoder();

    virtual ~HttpResponseDecoder();

    virtual DecodeResult decode(boost::asio::streambuf& buffer, HttpObject **object);

    virtual void reset() {
        result_ = kIncomplete;
        state_ = kResponseStart;
        if(temp_object_) {
            delete temp_object_;
            temp_object_ = nullptr;
        }
        body_length_ = 0;
        chunked_ = false;
    }

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

    bool ischar(int c);
    bool istspecial(int c);

    DecodeResult result_;
    DecodeState state_;

    HttpObject *temp_object_;
    std::size_t body_length_;
    bool chunked_;
};

#endif // HTTP_RESPONSE_DECODER_H
