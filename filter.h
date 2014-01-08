#ifndef FILTER_H
#define FILTER_H

#include <string>

class FilterChain;
class FilterContext;
class HttpContainer;

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

    enum FilterType {
        kBoth,
        kRequest,
        kResponse
    };

    enum { // pre-defined priorites
        kHighest = 999, kMiddle = 500, kLowest = 1
    };

    Filter(FilterType type = kBoth) : type_(type) {}

    virtual ~Filter() {}

    virtual FilterResult process(HttpContainer *in, HttpContainer **out = 0) = 0;

    virtual int priority() = 0;

    FilterType type() const { return type_; }

    virtual const std::string name() const {
        return "Filter";
    }

protected:
    FilterType type_;
};

#endif // FILTER_H
