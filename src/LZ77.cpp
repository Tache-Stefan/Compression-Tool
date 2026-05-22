#include "LZ77.h"

LZ77::LZ77() {
    _output_tokens.reserve(INITIAL_TOKEN_CAPACITY);
    _decompressed_data.reserve(INITIAL_TOKEN_CAPACITY);
    _hash_table.fill(-1);
}

std::span<const Token> LZ77::compress_block(std::span<const uint8_t> input) {
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

    return _output_tokens;
}

std::span<const uint8_t> LZ77::decompress_block(std::span<const Token> input) {
    _decompressed_data.clear();

    for (const auto& token : input) {
        if (std::holds_alternative<Literal>(token)) {
            _decompressed_data.push_back(std::get<Literal>(token).byte);
        } else {
            const auto& match = std::get<Match>(token);
            size_t start_idx = _decompressed_data.size() - match.distance;
            for (size_t i = 0; i < match.length; ++i) {
                _decompressed_data.push_back(_decompressed_data[start_idx + i]);
            }
        }
    }

    return _decompressed_data;
}

bool LZ77::is_valid_match(const std::span<const uint8_t>& input, int64_t candidate_idx, size_t cursor) noexcept {
    if (candidate_idx == -1 || (cursor - candidate_idx) > MAX_WINDOW_SIZE) {
        return false;
    }

    size_t bytes = 0;
    while (bytes < MIN_MATCH_LENGTH && input[candidate_idx + bytes] == input[cursor + bytes]) {
        ++bytes;
    }

    return bytes == MIN_MATCH_LENGTH;
}
