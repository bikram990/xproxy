#include "CommonTestDefinition.hpp"
#include "xproxy/memory/byte_buffer.hpp"
#include "xproxy/message/http/http_parser.hpp"
#include "xproxy/message/http/http_request.hpp"

using namespace xproxy::message::http;
using namespace xproxy::memory;

TEST(HttpParserTest, Request) {
    HttpRequest request;
    HttpParser parser(request, HTTP_REQUEST);

    ByteBuffer req1;
    req1 << "GET http://github.com/kelvinh HTTP/1.1\r\n"
         << "Host: github.com\r\n\r\n";

    EXPECT_EQ(req1.size(), 60);

    auto sz = parser.consume(req1.data(), req1.size());

    EXPECT_EQ(sz, req1.size());
}
