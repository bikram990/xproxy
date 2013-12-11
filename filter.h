#ifndef FILTER_H
#define FILTER_H

class HttpObject;

/**
 * @brief The Filter class
 *
 * Classes inherit from this interface are used to filter requests and responses
 */
class Filter {
public:
    enum FilterResult {
        kContinue, // go to next filter
        kSkip,     // skip all left filters
        kStop      // stop filtering, indicates there is error occurred
    };

    virtual ~Filter() {}

    virtual FilterResult process(const HttpObject& object) = 0;

    virtual int priority() = 0;
};

#endif // FILTER_H
