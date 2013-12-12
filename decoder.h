#ifndef DECODER_H
#define DECODER_H

namespace boost { namespace asio { class streambuf; } }

/**
 * @brief The Decoder class
 *
 * Classes inherit from this interface are used to decode byte content into object
 */
class Decoder {
public:
    enum DecodeResult {
        kFailure, kContinue, kFinished
    };

    virtual ~Decoder() {}

    virtual DecodeResult decode(boost::asio::streambuf& buffer) = 0;
};

#endif // DECODER_H
