#include "LZ77.h"
#include "Token.h"
#include "BitWriter.h"
#include "BitReader.h"

LZ77::LZ77() {
    _output_tokens.reserve(INITIAL_TOKEN_CAPACITY);
    _decompressed_data.reserve(INITIAL_TOKEN_CAPACITY);
    _hash_table.fill(-1);
}

std::vector<uint8_t> LZ77::compress_block(const std::span<const uint8_t>& input) {
    _output_tokens.clear();
    _hash_table.fill(-1);

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

    std::vector<uint8_t> packed_data;
    packed_data.reserve(input.size());
    BitWriter writer(packed_data);

    for (const auto& token : _output_tokens) {
        if (std::holds_alternative<Literal>(token)) {
            writer.write(0, 1); // Literal flag
            writer.write(std::get<Literal>(token).byte, 8);
        } else {
            const auto& match = std::get<Match>(token);
            writer.write(1, 1); // Match flag
            writer.write(match.distance, 16);
            writer.write(match.length, 8);
        }
    }

    writer.flush();
    return packed_data;
}

std::vector<uint8_t> LZ77::decompress_block(const std::span<const uint8_t>& input, const size_t original_size) {
    _decompressed_data.clear();
    _decompressed_data.reserve(original_size);

    std::vector<uint8_t> padded_input(input.begin(), input.end());
    padded_input.insert(padded_input.end(), 8, 0x00);

    BitReader reader(padded_input);

    while (_decompressed_data.size() < original_size) {
        uint32_t is_match = reader.read_bits(1);

        if (is_match == 0) {
            uint8_t byte = static_cast<uint8_t>(reader.read_bits(8));
            _decompressed_data.push_back(byte);
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
