#include "CommonTestDefinition.hpp"
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

    EXPECT_EQ(sbb.size(), 13);
    EXPECT_EQ(sbb.available(), 13);
    EXPECT_TRUE((ArraysMatch<char, 13>(sbb.data(), "rep12345!@#$%")));

    sbb.consume(3);
    EXPECT_EQ(sbb.size(), 13);
    EXPECT_EQ(sbb.available(), 10);
    EXPECT_TRUE((ArraysMatch<char, 10>(sbb.data(), "12345!@#$%")));

    EXPECT_TRUE(sbb.replace(1, data, 3) == ByteBuffer::npos);
}

TEST(SegmentalByteBufferTest, Seperate) {
    SegmentalByteBuffer sbb;
    sbb.append("abcde", true)
       .append("12345", true)
       .append("!@#$%", false);

    EXPECT_EQ(sbb.SegmentCount(), 2);

    EXPECT_TRUE(sbb.seperate(0) == ByteBuffer::npos);
    EXPECT_TRUE(sbb.seperate(100) == ByteBuffer::npos);

    EXPECT_EQ(sbb.seperate(10), 10);

    EXPECT_EQ(sbb.SegmentCount(), 3);

    char data[2] = {'x', 'y'};
    sbb.replace(2, data, 2);

    EXPECT_EQ(sbb.size(), 12);
    EXPECT_TRUE((ArraysMatch<char, 12>(sbb.data(), "abcdexy!@#$%")));

    sbb.consume(5);
    EXPECT_TRUE((ArraysMatch<char, 2>(sbb.data(), "xy")));

    EXPECT_EQ(sbb.seperate(9), 9);
    EXPECT_EQ(sbb.SegmentCount(), 4);
    sbb.replace(4, "OPQZYX", 6);

    EXPECT_EQ(sbb.size(), 15);
    EXPECT_EQ(sbb.available(), 10);

    EXPECT_TRUE((ArraysMatch<char, 10>(sbb.data(), "xy!@OPQZYX")));
}
