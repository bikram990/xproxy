#include "test.hpp"
#include "x/memory/byte_buffer.hpp"

using namespace x::memory;

TEST(test_byte_buffer, operator_equal) {
    byte_buffer bb1;
    byte_buffer bb2;

    EXPECT_TRUE(bb1 == bb2);

    bb1 << 1 << 2;
    bb2 << 1 << 2 << 'a';

    EXPECT_FALSE(bb1 == bb2);
}

TEST(test_byte_buffer, operator_assign) {
    byte_buffer bb1;
    byte_buffer bb2;
    bb1 << 'a' << 'b' << 'c';
    bb2 = bb1;

    EXPECT_TRUE(bb1 == bb2);
}

TEST(test_byte_buffer, operator_shift) {
    byte_buffer bb;
    bb << 'a' << "bcde" << 1;

    EXPECT_TRUE((arrays_match<char, 5>(bb.data(), "abcde1")));
}

TEST(test_byte_buffer, common) {
    byte_buffer bb;

    EXPECT_TRUE(bb.size() == 0);
    EXPECT_TRUE(bb.empty());

    bb << 1 << 2 << 3;

    EXPECT_TRUE(bb.size() == 3);
    EXPECT_TRUE(!bb.empty());
    EXPECT_TRUE(bb.data(3) == nullptr);

    byte_buffer aux(bb.data(1), bb.size() - 1);
    byte_buffer aux2;
    aux2 << "23";

    EXPECT_TRUE(aux == aux2);
}
