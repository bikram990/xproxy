#ifndef REQUEST_H
#define REQUEST_H

template<typename IterT>
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
    virtual BuildResult BuildFromRaw(IterT begin, IterT end) = 0;
    virtual ~Request() = 0 {}

protected:
    Request() {}
};

#endif // REQUEST_H
