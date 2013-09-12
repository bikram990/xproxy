#ifndef REQUEST_H
#define REQUEST_H

class Request {
public:
    enum BuildResult {
        kBuildError = 0,
        kNotStart,
        kNotComplete,
        kComplete
    };

    // the build process may repeats twice or more, if the request is too large
    // and are transferred twice or more times
    virtual BuildResult BuildFromRaw(char *buffer, std::size_t length) = 0;
    virtual ~Request() = 0;

protected:
    Request();
};

#endif // REQUEST_H
