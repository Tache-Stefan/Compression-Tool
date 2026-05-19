#include <gtest/gtest.h>
#include "BitReader.h"
#include "BitWriter.h"

TEST(BitReaderWriterTest, BasicReadWrite) {
    std::vector<uint8_t> buffer;
    BitWriter writer(buffer);

    writer.write(0b101, 3);
    writer.write(0b1101, 4);
    writer.write(0b11111111, 8);
    writer.flush();
    buffer.insert(buffer.end(), 8, 0x00);

    BitReader reader{std::span<const uint8_t>(buffer.data(), buffer.size())};
    
    EXPECT_EQ(reader.read_bits(3), 0b101);
    EXPECT_EQ(reader.read_bits(4), 0b1101);
    EXPECT_EQ(reader.read_bits(8), 0b11111111);
    EXPECT_EQ(reader.read_bits(8), 0b00000000);
}

TEST(BitReaderWriterTest, PeekAndAdvance) {
    std::vector<uint8_t> buffer;
    BitWriter writer(buffer);

    writer.write(0b10101010, 8);
    writer.flush();
    buffer.insert(buffer.end(), 8, 0x00);

    BitReader reader{std::span<const uint8_t>(buffer.data(), buffer.size())};
    
    EXPECT_EQ(reader.peek_bits(4), 0b1010);
    EXPECT_EQ(reader.read_bits(4), 0b1010);
    EXPECT_EQ(reader.peek_bits(4), 0b1010);
    reader.advance_bits(4);
    EXPECT_EQ(reader.read_bits(8), 0b00000000);
}

TEST(BitReaderWriterTest, EdgeCases) {
    std::vector<uint8_t> buffer;
    BitWriter writer(buffer);

    writer.write(0b1111, 0);
    writer.flush();
    writer.write(0xFFFFFFFF, 32);
    writer.flush();
    buffer.insert(buffer.end(), 8, 0x00);

    BitReader reader{std::span<const uint8_t>(buffer.data(), buffer.size())};
    
    EXPECT_EQ(reader.read_bits(0), 0);
    EXPECT_EQ(reader.read_bits(32), 0xFFFFFFFF);
}
