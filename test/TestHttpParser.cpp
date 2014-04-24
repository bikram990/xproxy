#include "CommonTestDefinition.hpp"
#include "http_parser.hpp"

TEST(HttpParserTest, Common) {
    std::shared_ptr<Connection> c;
    HttpParser parser(c, HTTP_REQUEST);
}
