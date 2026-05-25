#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

#pragma pack(push, 1)
struct ArchiveHeader {
    char magic[4] = {'Q', 'A', 'R', 'C'};
    uint8_t codec_id;
    uint64_t original_size;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct BlockHeader {
    uint32_t original_size;
    uint32_t compressed_size;
};
#pragma pack(pop)

class Engine {
public:
    Engine() = default;
    Engine(std::string mode, std::string input_path, std::string output_path, bool print_info = true) 
        : mode(std::move(mode)), input_path(std::move(input_path)), output_path(std::move(output_path)), print_info(print_info) {}

    inline void set_mode(const std::string& new_mode) { mode = new_mode; }
    inline void set_input_path(const std::string& new_input_path) { input_path = new_input_path; }
    inline void set_output_path(const std::string& new_output_path) { output_path = new_output_path; }
    inline void set_print_info(const bool new_print_info) { print_info = new_print_info; }

    void print_usage() const;
    void run() const;

private:
    std::string mode;
    std::string input_path;
    std::string output_path;
    bool print_info = true;

    void compress_parallel(const uint8_t codec_id) const;
    void decompress_parallel() const;
};
