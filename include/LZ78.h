#pragma once

#include <cstdint>
#include <vector>
#include <span>
#include <cassert>

#include "IsCodec.h"

#pragma pack(push, 1)
struct LZ78Token {
    uint32_t dict_idx;
    uint8_t next_char;
};
#pragma pack(pop)

class LZ78 {
public:
    LZ78();

    constexpr inline static uint8_t get_codec_id() noexcept { return 0x02; }
    [[nodiscard]] std::vector<uint8_t> compress_block(const std::span<const uint8_t>& input);
    [[nodiscard]] std::vector<uint8_t> decompress_block(const std::span<const uint8_t>& input, const size_t original_size);

private:
    static constexpr uint32_t MAX_DICT_SIZE = 1 << 20;
    static constexpr uint32_t HASH_TABLE_SIZE = 1 << 22;

    // [ 32-bit Child Index | 24-bit Parent Index | 8-bit Char ]
    std::vector<uint64_t> _hash_table;

    struct DictNode {
        uint32_t parent;
        uint8_t character;
        uint32_t length;
    };
    std::vector<DictNode> _decode_dict;
    
    [[nodiscard]] inline uint32_t hash(const uint64_t key) const noexcept {
        return (key * 2654435761ull) & (HASH_TABLE_SIZE - 1);
    }
};

static_assert(IsCodec<LZ78>, "LZ78 does not satisfy the IsCodec requirements.");
