#pragma once

#include <vector>
#include <cstdint>
#include <future>
#include <variant>

#include "LZ77.h"
#include "LZ78.h"

enum class JobType {
    Compress,
    Decompress
};

struct Result {
    size_t chunk_id;
    size_t original_size;
    std::vector<uint8_t> output_data;
};

struct Task {
    size_t chunk_id;
    JobType type;
    size_t original_size;
    std::vector<uint8_t> input_data;
    std::promise<Result> promise;
};

using CodecVariant = std::variant<LZ77, LZ78>;
