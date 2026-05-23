#pragma once

#include <cstdint>
#include <cstddef>

struct HuffmanNode {
    uint32_t frequency  = 0;
    int16_t symbol      = -1;  // 0-256 for leaf nodes, -1 for internal routing nodes
    size_t left_child  = -1;  // Index in the vector, -1 if none
    size_t right_child = -1;  // Index in the vector, -1 if none

    HuffmanNode() = default;
    HuffmanNode(uint32_t freq, int16_t sym) : frequency(freq), symbol(sym) {}
};

struct HuffmanCode {
    uint32_t bits  = 0;
    uint8_t length = 0;

    HuffmanCode() = default;
    HuffmanCode(uint32_t b, uint8_t l) : bits(b), length(l) {}
};
