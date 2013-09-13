#ifndef REQUEST_H
#define REQUEST_H

#include <boost/asio/streambuf.hpp>


class Request {
public:
    enum BuildResult {
        kBuildError = 0,
        kNotStart, // TODO this is not used
        kNotComplete,
        kComplete
    };

    // the build process may repeats twice or more, if the request is too large
    // and are transferred twice or more times
    virtual BuildResult BuildFromRaw(char *buffer, std::size_t length) = 0;
    virtual boost::asio::streambuf& OutboundBuffer() = 0;
    virtual ~Request() = 0;

protected:
    Request();
};

#endif // REQUEST_H
