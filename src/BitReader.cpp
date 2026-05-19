#include "BitReader.h"

BitReader::BitReader(std::span<const uint8_t> buffer) : source_buffer(buffer) {}

uint32_t BitReader::read_bits(const uint32_t bits_to_read) {
    ensure_bits(bits_to_read);

    uint64_t mask = (1ULL << bits_to_read) - 1;
    uint32_t res = static_cast<uint32_t>(bit_buffer & mask);
    bit_buffer >>= bits_to_read;
    bit_count -= bits_to_read;

    return res;
}

uint32_t BitReader::peek_bits(const uint32_t bits_to_peek) {
    ensure_bits(bits_to_peek);

    uint64_t mask = (1ULL << bits_to_peek) - 1;
    return static_cast<uint32_t>(bit_buffer & mask);
}

void BitReader::advance_bits(const uint32_t bits_to_advance) noexcept {
    assert(bit_count >= bits_to_advance && "Cannot advance more bits than available");

    bit_buffer >>= bits_to_advance;
    bit_count -= bits_to_advance;
}

void BitReader::ensure_bits(const uint32_t bits_needed) {
    assert(bits_needed <= 32 && "Cannot read more than 32 bits");

    while (bit_count < bits_needed) {
        bit_buffer |= static_cast<uint64_t>(source_buffer[byte_index++]) << bit_count;
        bit_count += 8;
    }
}
