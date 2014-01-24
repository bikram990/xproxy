#ifndef FILTER_H
#define FILTER_H

#include <memory>
#include <string>

class FilterChain;
class FilterContext;
class HttpContainer;
struct SessionContext;

/**
 * @brief The Filter class
 *
 * Classes inherit from this interface are used to filter requests and responses
 */
class Filter {
public:
    typedef std::shared_ptr<Filter> Ptr;

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

    virtual FilterResult process(SessionContext& context, FilterType type) = 0;

    virtual int priority() = 0;

    FilterType type() const { return type_; }

    virtual const std::string name() const {
        return "Filter";
    }

protected:
    FilterType type_;
};

#endif // FILTER_H
