#pragma once

#include <span>
#include <cassert>
#include <cstdint>
#include <stdexcept>

class BitReader {
public:
    explicit BitReader(std::span<const uint8_t> buffer);

    uint32_t read_bits(const uint32_t bits_to_read);
    uint32_t peek_bits(const uint32_t bits_to_peek);
    void advance_bits(const uint32_t bits_to_advance) noexcept;

private:
    std::span<const uint8_t> source_buffer;
    size_t byte_index = 0;
    uint64_t bit_buffer = 0;
    uint32_t bit_count = 0;

    void ensure_bits(const uint32_t bits_needed);
};
