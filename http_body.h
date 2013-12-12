#ifndef HTTP_BODY_H
#define HTTP_BODY_H

#include <vector>

class HttpChunk;

/**
 * @brief The HttpBody class
 *
 * This class is to combine all http chunks together, it is just a container
 * class, so it does not inherit from HttpObject.
 */
class HttpBody {
private:
    std::vector<HttpChunk> chunks_;
};

#endif // HTTP_BODY_H
