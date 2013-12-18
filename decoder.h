#ifndef DECODER_H
#define DECODER_H

#include <boost/asio.hpp>
#include "resettable.h"

class HttpObject;

/**
 * @brief The Decoder class
 *
 * Classes inherit from this interface are used to decode byte content into object
 */
class Decoder : public Resettable {
public:
    enum DecodeResult {
        kFailure,    // decoding failed, invalid data
        kIncomplete, // need more data to decode an object
        kComplete,   // an object is completed, but more objects are expected
        kFinished    // all objects are decoded without any error
    };

    virtual ~Decoder() {}

    virtual DecodeResult decode(boost::asio::streambuf& buffer, HttpObject **object) = 0;
};

#endif // DECODER_H
