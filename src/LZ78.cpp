#include "LZ78.h"

#include <cstring>
#include <stdexcept>

LZ78::LZ78() {
    _hash_table.resize(HASH_TABLE_SIZE, 0);
    _decode_dict.reserve(MAX_DICT_SIZE);
}

std::vector<uint8_t> LZ78::compress_block(const std::span<const uint8_t>& input) {
    if (input.empty()) { return {}; }

    std::memset(_hash_table.data(), 0, HASH_TABLE_SIZE * sizeof(uint64_t));
    
    std::vector<LZ78Token> tokens;
    tokens.reserve(input.size() / 2);

    uint32_t current_node = 0;
    uint32_t next_node_idx = 1;

    for (uint8_t c : input) {
        const uint64_t key = (static_cast<uint64_t>(current_node) << 8) | c;
        uint32_t slot = hash(key);

        while (true) {
            const uint64_t entry = _hash_table[slot];

            if (entry == 0) {
                tokens.emplace_back(current_node, c);
                if (next_node_idx < MAX_DICT_SIZE) {
                    _hash_table[slot] = (static_cast<uint64_t>(next_node_idx) << 32) | key;
                    ++next_node_idx;
                }

                current_node = 0;
                break;
            }
            if ((entry & 0xFFFFFFFF) == key) {
                current_node = entry >> 32;
                break;
            }

            slot = (slot + 1) & (HASH_TABLE_SIZE - 1);
        }
    }

    if (current_node != 0) {
        tokens.emplace_back(current_node, 0);
    }

    std::vector<uint8_t> compressed_data(tokens.size() * sizeof(LZ78Token));
    std::memcpy(compressed_data.data(), tokens.data(), compressed_data.size());

    return compressed_data;
}

std::vector<uint8_t> LZ78::decompress_block(const std::span<const uint8_t>& input, const size_t original_size) {
    if (input.empty()) return {};

    const size_t num_tokens = input.size() / sizeof(LZ78Token);
    const auto* tokens = reinterpret_cast<const LZ78Token*>(input.data());

    std::vector<uint8_t> output;
    output.reserve(original_size + 1);

    uint32_t next_node_idx = 1;

    _decode_dict[0] = {0, 0, 0};

    for (size_t i = 0; i < num_tokens; ++i) {
        const LZ78Token token = tokens[i];
    
        const uint32_t str_len = _decode_dict[token.dict_idx].length;
        const size_t current_out_size = output.size();
        output.resize(current_out_size + str_len + 1); 
        
        size_t write_pos = current_out_size + str_len;
        output[write_pos] = token.next_char;

        uint32_t curr = token.dict_idx;
        while (curr != 0) {
            if (curr >= MAX_DICT_SIZE || write_pos == 0 || write_pos > output.size()) {
                throw std::runtime_error("Invalid compressed data.");
            }
            output[--write_pos] = _decode_dict[curr].character;
            curr = _decode_dict[curr].parent;
        }

        if (next_node_idx < MAX_DICT_SIZE) {
            _decode_dict[next_node_idx++] = { token.dict_idx, token.next_char, str_len + 1 };
        }
    }

    output.resize(original_size);
    return output;
}
