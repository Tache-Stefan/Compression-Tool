#pragma once

#include <vector>
#include <cstdint>
#include <future>

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
