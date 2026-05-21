#pragma once

#include <cstdint>
#include <variant>

struct Literal {
    uint8_t byte = 0;

    Literal() = default;
    Literal(uint8_t byte) : byte(byte) {}
};

struct Match {
    uint16_t distance = 0;
    uint8_t length = 0;
    
    Match() = default;
    Match(uint16_t distance, uint8_t length) : distance(distance), length(length) {}
};

using Token = std::variant<Literal, Match>;
