#pragma once

#include <concepts>
#include <span>
#include <vector>
#include <cstdint>

template <typename T>
concept IsCodec = requires(T a, std::span<const uint8_t> input, size_t original_size) {
    { a.get_codec_id() } -> std::same_as<uint8_t>;
    
    { a.compress_block(input) } -> std::same_as<std::vector<uint8_t>>;
    
    { a.decompress_block(input, original_size) } -> std::same_as<std::vector<uint8_t>>;
};
