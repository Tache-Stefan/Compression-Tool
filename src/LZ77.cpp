#include "LZ77.h"

#include <queue>

#include "BitWriter.h"
#include "BitReader.h"

LZ77::LZ77() {
    _output_tokens.reserve(INITIAL_TOKEN_CAPACITY);
    _decompressed_data.reserve(INITIAL_TOKEN_CAPACITY);
    _hash_table.fill(-1);
    huffman_tree.reserve(513);
}

std::vector<uint8_t> LZ77::compress_block(const std::span<const uint8_t>& input) {
    if (input.empty()) return {};

    _hash_table.fill(-1);
    _codebook.fill(HuffmanCode());

    build_output_tokens(input);
    auto freq = calc_frequencies();

    auto root_idx = build_huffman_tree(freq);
    generate_codes(root_idx, 0, 0);

    std::vector<uint8_t> packed_data;
    packed_data.reserve(input.size());
    
    write_compression_bits(freq, packed_data);

    return packed_data;
}

std::vector<uint8_t> LZ77::decompress_block(const std::span<const uint8_t>& input, const size_t original_size) {
    _decompressed_data.clear();
    _decompressed_data.reserve(original_size);

    std::vector<uint8_t> padded_input(input.begin(), input.end());
    padded_input.insert(padded_input.end(), 8, 0x00);

    BitReader reader(padded_input);
    std::array<uint32_t, 257> freq;
    for (size_t i = 0; i < freq.size(); ++i) {
        freq[i] = reader.read_bits(32);
    }

    auto root_idx = build_huffman_tree(freq);

    while (_decompressed_data.size() < original_size) {
        size_t node_idx = root_idx;

        while (huffman_tree[node_idx].symbol == -1) {
            uint32_t bit = reader.read_bits(1);
            node_idx = bit == 0 ? huffman_tree[node_idx].left_child : huffman_tree[node_idx].right_child;
        }

        auto symbol = huffman_tree[node_idx].symbol;
        if (symbol < 256) {
            _decompressed_data.push_back(static_cast<uint8_t>(symbol));
        } else {
            uint16_t distance = static_cast<uint16_t>(reader.read_bits(16));
            uint8_t length = static_cast<uint8_t>(reader.read_bits(8));

            size_t start_idx = _decompressed_data.size() - distance;
            for (size_t i = 0; i < length; ++i) {
                _decompressed_data.push_back(_decompressed_data[start_idx + i]);
            }
        }
    }

    return _decompressed_data;
}

bool LZ77::is_valid_match(const std::span<const uint8_t>& input, const int64_t candidate_idx, const size_t cursor) noexcept {
    if (candidate_idx == -1 || (cursor - candidate_idx) > MAX_WINDOW_SIZE) {
        return false;
    }

    size_t bytes = 0;
    while (bytes < MIN_MATCH_LENGTH && input[candidate_idx + bytes] == input[cursor + bytes]) {
        ++bytes;
    }

    return bytes == MIN_MATCH_LENGTH;
}

void LZ77::generate_codes(const size_t node_idx, uint32_t current_path, uint8_t current_depth) noexcept {
    const auto& node = huffman_tree[node_idx];

    if (node.symbol != -1) {
        _codebook[node.symbol] = {current_path, current_depth};
        return;
    }

    generate_codes(node.left_child, current_path, current_depth + 1);
    generate_codes(node.right_child, current_path | (1U << current_depth), current_depth + 1);
}

void LZ77::build_output_tokens(const std::span<const uint8_t>& input) {
    _output_tokens.clear();
    _output_tokens.reserve(input.size() / 2);

    for (size_t cursor = 0; cursor < input.size(); ++cursor) {
        if (cursor + 2 >= input.size()) {
            _output_tokens.emplace_back(Literal(input[cursor]));
            continue;
        }

        int64_t h = hash(&input[cursor]);
        int64_t candidate_idx = _hash_table[h];
        _hash_table[h] = cursor;

        if (!is_valid_match(input, candidate_idx, cursor)) {
            _output_tokens.emplace_back(Literal(input[cursor]));
            continue;
        }

        size_t bytes = 0;
        while (bytes < MAX_MATCH_LENGTH && (cursor + bytes) < input.size() && 
               input[candidate_idx + bytes] == input[cursor + bytes]) {
            ++bytes;
        }
        _output_tokens.emplace_back(Match(cursor - candidate_idx, bytes));

        for (size_t i = 1; i < bytes; ++i) {
            if (cursor + i + 2 < input.size()) {
                int64_t step_h = hash(&input[cursor + i]);
                _hash_table[step_h] = cursor + i;
            }
        }
        cursor += bytes - 1;
    }
}

std::array<uint32_t, 257> LZ77::calc_frequencies() const noexcept {
    std::array<uint32_t, 257> freq{};

    for (const auto& token : _output_tokens) {
        if (std::holds_alternative<Literal>(token)) {
            uint8_t byte = std::get<Literal>(token).byte;
            ++freq[byte];
        } else {
            ++freq[256];
        }
    }
    freq[256] = std::max(freq[256], static_cast<uint32_t>(1));

    return freq;
}

size_t LZ77::build_huffman_tree(const std::array<uint32_t, 257>& freq) {
    huffman_tree.clear();

    for (size_t i = 0; i < freq.size(); ++i) {
        if (freq[i] > 0) {
            huffman_tree.emplace_back(freq[i], static_cast<int16_t>(i));
        }
    }

    auto cmp = [this](size_t a, size_t b) {
        return huffman_tree[a].frequency > huffman_tree[b].frequency;
    };
    std::priority_queue<size_t, std::vector<size_t>, decltype(cmp)> pq(cmp);
    for (size_t i = 0; i < huffman_tree.size(); ++i) {
        pq.push(i);
    }

    while (pq.size() > 1) {
        size_t left_idx = pq.top(); pq.pop();
        size_t right_idx = pq.top(); pq.pop();

        huffman_tree.emplace_back(huffman_tree[left_idx].frequency + huffman_tree[right_idx].frequency, -1);
        huffman_tree.back().left_child = left_idx;
        huffman_tree.back().right_child = right_idx;
        pq.push(huffman_tree.size() - 1);
    }

    return pq.top();
}

void LZ77::write_compression_bits(const std::array<uint32_t, 257>& freq, std::vector<uint8_t>& packed_data) {
    BitWriter writer(packed_data);

    for (size_t i = 0; i < 257; ++i) {
        writer.write(freq[i], 32);
    }
    for (const auto& token : _output_tokens) {
        if (std::holds_alternative<Literal>(token)) {
            uint8_t byte = std::get<Literal>(token).byte;
            const auto& code = _codebook[byte];
            writer.write(code.bits, code.length);
        } else {
            const auto& code = _codebook[256];
            writer.write(code.bits, code.length);
            const auto& match = std::get<Match>(token);
            writer.write(match.distance, 16);
            writer.write(match.length, 8);
        }
    }

    writer.flush();
}
