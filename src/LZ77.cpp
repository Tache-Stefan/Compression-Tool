#include "LZ77.h"

LZ77::LZ77() {
    _output_tokens.reserve(INITIAL_TOKEN_CAPACITY);
    _decompressed_data.reserve(INITIAL_TOKEN_CAPACITY);
}

std::span<const Token> LZ77::compress_block(std::span<const uint8_t> input) {
    _output_tokens.clear();

    for (size_t cursor = 0; cursor < input.size(); ++cursor) {
        const int64_t loopback_limit = (cursor > MAX_WINDOW_SIZE) ? cursor - MAX_WINDOW_SIZE : 0;
        uint16_t max_bytes = 0;
        int64_t best_back_idx = 0;

        for (int64_t back_idx = cursor - 1; back_idx >= loopback_limit; --back_idx) {
            uint16_t bytes = 0;

            while ((cursor + bytes) < input.size() && 
                    input[cursor + bytes] == input[back_idx + bytes] && 
                    bytes < MAX_MATCH_LENGTH) {
                ++bytes;
            }

            if (bytes > max_bytes) {
                max_bytes = bytes;
                best_back_idx = back_idx;
            }
        }

        if (max_bytes >= MIN_MATCH_LENGTH) {
            uint16_t distance = cursor - best_back_idx;
            _output_tokens.emplace_back(std::in_place_type<Match>, distance, max_bytes);
            cursor += max_bytes - 1;
        } else {
            _output_tokens.emplace_back(std::in_place_type<Literal>, input[cursor]);
        }
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
