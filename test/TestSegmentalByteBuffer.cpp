#include "gtest/gtest.h"
#include "memory/segmental_byte_buffer.hpp"

TEST(SegmentalByteBufferTest, Common) {
    SegmentalByteBuffer sbb;

    EXPECT_TRUE(sbb.empty());
    EXPECT_TRUE(sbb.size() == 0);
    EXPECT_TRUE(sbb.SegmentCount() == 0);
    EXPECT_TRUE(sbb.available() == 0);

    sbb.append("abc", true);

    EXPECT_FALSE(sbb.empty());
    EXPECT_TRUE(sbb.size() == 3);
    EXPECT_TRUE(sbb.SegmentCount() == 1);
    EXPECT_TRUE(sbb.available() == 3);

    sbb.consume(1);

    EXPECT_TRUE(sbb.size() == 3);
    EXPECT_TRUE(sbb.SegmentCount() == 1);
    EXPECT_TRUE(sbb.available() == 2);

    sbb.append("def", true);

    EXPECT_TRUE(sbb.size() == 6);
    EXPECT_TRUE(sbb.SegmentCount() == 2);
    EXPECT_TRUE(sbb.available() == 5);

    sbb.append("gh", false);

    EXPECT_TRUE(sbb.SegmentCount() == 2);
}

TEST(SegmentalByteBufferTest, Replace) {
    SegmentalByteBuffer sbb;
    sbb.append("abcde", true)
       .append("12345", true)
       .append("!@#$%", true);

    EXPECT_TRUE(sbb.SegmentCount() == 3);

    char data[3] = {'r', 'e', 'p'};

    sbb.replace(1, data, 3);

    EXPECT_TRUE(sbb.size() == 13);
}
