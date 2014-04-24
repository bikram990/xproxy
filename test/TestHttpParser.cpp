#include "CommonTestDefinition.hpp"
#include "http_parser.hpp"

TEST(HttpParserTest, Request) {
    std::shared_ptr<Connection> c;
    HttpParser parser(c, HTTP_REQUEST);

    boost::asio::streambuf req1;
    std::ostream out(&req1);
    out << "GET http://github.com/kelvinh HTTP/1.1\r\n"
        << "Host: github.com\r\n\r\n";

    EXPECT_EQ(req1.size(), 60);
    EXPECT_EQ(parser.consume(req1), req1.size());
}
