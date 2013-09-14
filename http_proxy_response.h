#ifndef HTTP_PROXY_RESPONSE_H
#define HTTP_PROXY_RESPONSE_H

#include <string>
#include <vector>
#include <boost/array.hpp>


class HttpProxyResponse {
public:
    HttpProxyResponse();
    ~HttpProxyResponse();

    std::string& status_line() { return status_line_; }
    boost::array<char, 4096>& body() { return body_; }

private:
    struct Header {
        std::string name;
        std::string value;
    };

    std::string status_line_;
    std::vector<Header> headers_;
    boost::array<char, 4096> body_;
};

#endif // HTTP_PROXY_RESPONSE_H
