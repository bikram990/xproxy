#include "gtest/gtest.h"
#include "memory/byte_buffer.hpp"

TEST(ByteBufferTest, Basic) {
    ByteBuffer bb1;
    ByteBuffer bb2;

    EXPECT_TRUE(bb1 == bb2);
}
