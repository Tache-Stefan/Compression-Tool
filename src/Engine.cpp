#include "Engine.h"

#include <chrono>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <print>

#include "LZ77.h"

void Engine::run() const {
    if (mode == "-lz77") {
        std::println("[*] Reading {}...", input_path);
        auto input_data = read_file(input_path);
        
        std::println("[*] Compressing with LZ77...");
        LZ77 engine;

        auto start_time = std::chrono::high_resolution_clock::now();
        auto compressed_data = engine.compress_block(input_data);
        auto end_time = std::chrono::high_resolution_clock::now();

        ArchiveHeader header;
        header.codec_id = engine.get_codec_id();
        header.original_size = input_data.size();

        std::ofstream out_file(output_path, std::ios::binary);
        out_file.write(reinterpret_cast<const char*>(&header), sizeof(ArchiveHeader));
        out_file.write(reinterpret_cast<const char*>(compressed_data.data()), compressed_data.size());

        std::chrono::duration<double> elapsed_seconds = end_time - start_time;
        double input_mb = static_cast<double>(input_data.size()) / (1024.0 * 1024.0);
        double throughput = input_mb / elapsed_seconds.count();

        std::println("[+] Done! Compressed {} bytes down to {} bytes.", 
                        input_data.size(), compressed_data.size() + sizeof(ArchiveHeader));
        std::println("[*] Compression took {:.4f} seconds. Throughput: {:.2f} MB/s",
                        elapsed_seconds.count(), throughput);
    } else if (mode == "-x") {
        std::println("[*] Reading Archive {}...", input_path);
        auto archive_data = read_file(input_path);
        
        if (archive_data.size() < sizeof(ArchiveHeader)) {
            throw std::runtime_error("File is too small to be a valid archive.");
        }

        ArchiveHeader header;
        std::memcpy(&header, archive_data.data(), sizeof(ArchiveHeader));

        if (std::strncmp(header.magic, "QARC", 4) != 0) {
            throw std::runtime_error("Invalid magic bytes. Not a QARC archive!");
        }

        switch (header.codec_id) {
            case 0x01: {
                std::println("[*] Detected LZ77 Archive. Decompressing...");
                LZ77 engine;
                
                std::span<const uint8_t> payload(archive_data.data() + sizeof(ArchiveHeader), 
                                                 archive_data.size() - sizeof(ArchiveHeader));
                
                auto decompressed_data = engine.decompress_block(payload, header.original_size);
                
                write_file(output_path, decompressed_data);
                std::println("[+] Extracted {} bytes successfully.", decompressed_data.size());
                break;
            }
            default:
                throw std::runtime_error("Unknown Codec ID in archive.");
        }
    } else {
        std::println("Unknown command: {}", mode);
        print_usage();
    }
}

std::vector<uint8_t> Engine::read_file(const std::string& filepath) const {
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file) throw std::runtime_error("Could not open file for reading: " + filepath);
    
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<uint8_t> buffer(size);
    if (file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        return buffer;
    }
    throw std::runtime_error("Failed to read file contents.");
}

void Engine::write_file(const std::string& filepath, const std::vector<uint8_t>& data) const {
    std::ofstream file(filepath, std::ios::binary);
    if (!file) throw std::runtime_error("Could not open file for writing: " + filepath);
    file.write(reinterpret_cast<const char*>(data.data()), data.size());
}

void Engine::print_usage() const {
    std::println("Usage:\n"
                "  Compress: ./quant_archiver -lz77 <input_file> <output_archive>\n"
                "  Extract:  ./quant_archiver -x <input_archive> <output_file>");
}
