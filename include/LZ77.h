#pragma once

#include <cstdint>
#include <span>
#include <vector>

#include "Token.h"

class LZ77 {
public:
    LZ77();

    std::span<const Token> compress_block(std::span<const uint8_t> input);
    std::span<const uint8_t> decompress_block(std::span<const Token> input);

private:
    static constexpr size_t MIN_MATCH_LENGTH       = 3;
    static constexpr size_t MAX_MATCH_LENGTH       = 255;
    static constexpr size_t MAX_WINDOW_SIZE        = 65'535;
    static constexpr size_t INITIAL_TOKEN_CAPACITY = 10'000;
    std::vector<Token> _output_tokens;
    std::vector<uint8_t> _decompressed_data;
};
