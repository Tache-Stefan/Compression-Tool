#pragma once

#include <cassert>
#include <cstdint>
#include <vector>

class BitWriter {
public:
    explicit BitWriter(std::vector<uint8_t>& buffer);

    void write(uint32_t value, const uint32_t bits_to_write);
    void flush();

private:
    std::vector<uint8_t>& target_buffer;
    uint64_t bit_buffer = 0;
    uint32_t bit_count = 0;
};
