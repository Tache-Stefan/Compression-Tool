#include "BitWriter.h"

BitWriter::BitWriter(std::vector<uint8_t>& buffer) : target_buffer(buffer) {}

void BitWriter::write(uint32_t value, const uint32_t bits_to_write) {
    assert(bits_to_write <= 32 && "Cannot write more than 32 bits at once");

    // Ensure no dirty out-of-bounds bits exist
    uint64_t mask = (1ULL << bits_to_write) - 1;
    value &= static_cast<uint32_t>(mask);

    bit_buffer |= static_cast<uint64_t>(value) << bit_count;
    bit_count += bits_to_write;

    while (bit_count >= 8) {
        target_buffer.push_back(static_cast<uint8_t>(bit_buffer & 0xFF));
        bit_buffer >>= 8;
        bit_count -= 8;
    }
}

void BitWriter::flush() {
    if (bit_count > 0) {
        target_buffer.push_back(static_cast<uint8_t>(bit_buffer & 0xFF));
        bit_buffer = 0;
        bit_count = 0;
    }
}
