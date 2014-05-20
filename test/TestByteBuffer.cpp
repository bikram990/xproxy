#include "CommonTestDefinition.hpp"
#include "memory/byte_buffer.hpp"

using namespace xproxy::memory;

TEST(ByteBufferTest, OperatorEqual) {
    ByteBuffer bb1;
    ByteBuffer bb2;

    EXPECT_TRUE(bb1 == bb2);

    bb1 << 1 << 2;
    bb2 << 1 << 2 << 'a';

    EXPECT_FALSE(bb1 == bb2);
}

TEST(ByteBufferTest, OperatorAssign) {
    ByteBuffer bb1;
    ByteBuffer bb2;
    bb1 << 'a' << 'b' << 'c';
    bb2 = bb1;

    EXPECT_TRUE(bb1 == bb2);
}

TEST(ByteBufferTest, OperatorShift) {
    ByteBuffer bb;
    bb << 'a' << "bcde" << 1;

    EXPECT_TRUE((ArraysMatch<char, 5>(bb.data(), "abcde1")));
}

TEST(ByteBufferTest, Common) {
    ByteBuffer bb;

    EXPECT_TRUE(bb.size() == 0);
    EXPECT_TRUE(bb.empty());

    bb << 1 << 2 << 3;

    EXPECT_TRUE(bb.size() == 3);
    EXPECT_TRUE(!bb.empty());
    EXPECT_TRUE(bb.data(3) == nullptr);

    ByteBuffer aux(bb.data(1), bb.size() - 1);
    ByteBuffer aux2;
    aux2 << "23";

    EXPECT_TRUE(aux == aux2);
}
