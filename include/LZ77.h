#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <vector>

#include "IsCodec.h"

class LZ77 {
public:
    LZ77();

    inline uint8_t get_codec_id() const noexcept { return 0x01; }
    std::vector<uint8_t> compress_block(const std::span<const uint8_t>& input);
    std::vector<uint8_t> decompress_block(const std::span<const uint8_t>& input, const size_t original_size);

private:
    static constexpr size_t MIN_MATCH_LENGTH       = 3;
    static constexpr size_t MAX_MATCH_LENGTH       = 255;
    static constexpr size_t MAX_WINDOW_SIZE        = 65'535;
    static constexpr size_t HASH_SIZE              = 65'536;
    static constexpr size_t INITIAL_TOKEN_CAPACITY = 10'000;
    std::vector<Token> _output_tokens;
    std::vector<uint8_t> _decompressed_data;
    std::array<int64_t, HASH_SIZE> _hash_table;

    inline int64_t hash(const uint8_t* p) noexcept {
        int64_t h = (p[0] << 10) ^ (p[1] << 5) ^ p[2];
        return h & (HASH_SIZE - 1);
    }
    bool is_valid_match(const std::span<const uint8_t>& input, const int64_t candidate_idx, const size_t cursor) noexcept;
};

static_assert(IsCodec<LZ77>, "LZ77 does not satisfy the IsCodec requirements.");
