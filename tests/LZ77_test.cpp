#include <gtest/gtest.h>
#include <span>

#include "LZ77.h"

TEST(LZ77Test, CompressAndDecompress) {
    LZ77 lz77;

    std::vector<uint8_t> input_data = { 'H', 'E', 'L', 'L', 'O', ' ', 'W', 'O', 'R', 'L', 'D', '!', ' ', 'H', 'E', 'L', 'L', 'O' };
    auto compressed = lz77.compress_block(input_data);
    auto decompressed = lz77.decompress_block(compressed, input_data.size());

    EXPECT_EQ(input_data.size(), decompressed.size());
    for (size_t i = 0; i < input_data.size(); ++i) {
        EXPECT_EQ(input_data[i], decompressed[i]);
    }
}

TEST(LZ77Test, EmptyCompressAndDecompress) {
    LZ77 lz77;

    std::vector<uint8_t> input_data;
    auto compressed = lz77.compress_block(input_data);
    auto decompressed = lz77.decompress_block(compressed, input_data.size());

    EXPECT_TRUE(compressed.empty());
    EXPECT_TRUE(decompressed.empty());
}

TEST(LZ77Test, NoMatches) {
    LZ77 lz77;

    std::vector<uint8_t> input_data = { 'A', 'B', 'C', 'D', 'E' };
    auto compressed = lz77.compress_block(input_data);
    auto decompressed = lz77.decompress_block(compressed, input_data.size());

    EXPECT_EQ(input_data.size(), decompressed.size());
    for (size_t i = 0; i < input_data.size(); ++i) {
        EXPECT_EQ(input_data[i], decompressed[i]);
    }
}
