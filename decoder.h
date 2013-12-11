#ifndef DECODER_H
#define DECODER_H

class SharedBuffer;

/**
 * @brief The Decoder class
 *
 * Classes inherit this interface are used to decode binary content into object
 */
class Decoder {
public:
    enum DecodeState {
        kFailure, kContinue, kFinished
    };

    virtual ~Decoder() {}

    virtual DecodeState decode(const SharedBuffer& buffer) = 0;
};

#endif // DECODER_H
