#pragma once

#include <array>
#include <cstdint>
#include <cstddef>
#include <span>
#include <vector>

#include "Huffman.h"
#include "IsCodec.h"
#include "Token.h"

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

    std::vector<HuffmanNode> huffman_tree;
    std::array<HuffmanCode, 257> _codebook;

    inline int64_t hash(const uint8_t* p) noexcept {
        int64_t h = (p[0] << 10) ^ (p[1] << 5) ^ p[2];
        return h & (HASH_SIZE - 1);
    }

    [[nodiscard]] bool is_valid_match(const std::span<const uint8_t>& input, const int64_t candidate_idx, const size_t cursor) noexcept;
    void generate_codes(const size_t node_idx, uint32_t current_path, uint8_t current_depth) noexcept;
    void build_output_tokens(const std::span<const uint8_t>& input);
    [[nodiscard]] std::array<uint32_t, 257> calc_frequencies() const noexcept;
    [[nodiscard]] size_t build_huffman_tree(const std::array<uint32_t, 257>& freq);
    void write_compression_bits(const std::array<uint32_t, 257>& freq, std::vector<uint8_t>& packed_data);
    [[nodiscard]] size_t bytes_matched(const std::span<const uint8_t>& input, const int64_t candidate_idx, const size_t cursor);
};

static_assert(IsCodec<LZ77>, "LZ77 does not satisfy the IsCodec requirements.");
