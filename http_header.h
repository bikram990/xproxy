#ifndef HTTP_HEADER_H
#define HTTP_HEADER_H

struct HttpHeader {
    std::string name;
    std::string value;

    HttpHeader(const std::string& name = "",
               const std::string& value = "")
        : name(name), value(value) {}
};

#endif // HTTP_HEADER_H
