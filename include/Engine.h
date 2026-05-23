#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

#pragma pack(push, 1)
struct ArchiveHeader {
    char magic[4] = {'Q', 'A', 'R', 'C'};
    uint8_t codec_id;
    size_t original_size;
};
#pragma pack(pop)

class Engine {
public:
    Engine() = default;
    Engine(std::string mode, std::string input_path, std::string output_path) 
        : mode(std::move(mode)), input_path(std::move(input_path)), output_path(std::move(output_path)) {}

    inline void set_mode(const std::string& new_mode) { mode = new_mode; }
    inline void set_input_path(const std::string& new_input_path) { input_path = new_input_path; }
    inline void set_output_path(const std::string& new_output_path) { output_path = new_output_path; }
    
    void print_usage() const;
    void run() const;

private:
    std::string mode;
    std::string input_path;
    std::string output_path;

    std::vector<uint8_t> read_file(const std::string& filepath) const;
    void write_file(const std::string& filepath, const std::vector<uint8_t>& data) const;
};
