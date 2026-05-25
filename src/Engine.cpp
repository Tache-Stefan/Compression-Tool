#include "Engine.h"

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <chrono>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <print>
#include <queue>

#include "LZ77.h"
#include "ThreadPool.h"

void Engine::run() const {
    if (mode == "-lz77") {
        compress_parallel(LZ77::get_codec_id());
    } else if (mode == "-lz78") {
        compress_parallel(LZ78::get_codec_id());
    } else if (mode == "-x") {
        decompress_parallel();
    } else {
        std::println("[!] Unknown command: {}", mode);
        print_usage();
    }
}

void Engine::compress_parallel(const uint8_t codec_id) const {
    const auto start_time = std::chrono::high_resolution_clock::now();

    int fd_in = open(input_path.c_str(), O_RDONLY);
    if (fd_in < 0) throw std::runtime_error("Cannot open input file.");
    
    struct stat sb;
    if (fstat(fd_in, &sb) < 0) throw std::runtime_error("Cannot stat input file.");
    const size_t in_size = sb.st_size;
    
    uint8_t* in_map = nullptr;
    if (in_size > 0) {
        in_map = static_cast<uint8_t*>(mmap(nullptr, in_size, PROT_READ, MAP_PRIVATE, fd_in, 0));
        if (in_map == MAP_FAILED) throw std::runtime_error("Failed to mmap input file.");
    }

    std::ofstream out_file(output_path, std::ios::binary);
    if (!out_file) throw std::runtime_error("Cannot open output file: " + output_path);
    
    ArchiveHeader header{'Q', 'A', 'R', 'C', codec_id, static_cast<uint64_t>(in_size)};
    out_file.write(reinterpret_cast<const char*>(&header), sizeof(header));

    const size_t num_threads = std::thread::hardware_concurrency();
    ThreadPool pool(num_threads, codec_id);
    std::queue<std::future<Result>> futures;

    const size_t CHUNK_SIZE = 1024 * 1024; // 1 MB
    const size_t MAX_IN_FLIGHT = num_threads * 2;
    
    size_t chunk_id = 0;
    size_t total_compressed_size = 0;
    size_t in_offset = 0;

    if (print_info) {
        std::println("[*] Compressing with {} threads...", num_threads);
    }

    while (in_offset < in_size || !futures.empty()) {
        while (in_offset < in_size && futures.size() < MAX_IN_FLIGHT) {
            const size_t bytes_to_read = std::min(CHUNK_SIZE, in_size - in_offset);

            std::vector<uint8_t> buffer(in_map + in_offset, in_map + in_offset + bytes_to_read);
            in_offset += bytes_to_read;

            futures.push(pool.enqueue_chunk(
                chunk_id++, 
                JobType::Compress, 
                bytes_to_read, 
                std::move(buffer)
            ));
        }

        if (!futures.empty()) {
            Result res = futures.front().get();
            futures.pop();
            
            BlockHeader block_hdr { 
                static_cast<uint32_t>(res.original_size), 
                static_cast<uint32_t>(res.output_data.size()) 
            };
            
            out_file.write(reinterpret_cast<const char*>(&block_hdr), sizeof(block_hdr));
            out_file.write(reinterpret_cast<const char*>(res.output_data.data()), res.output_data.size());

            total_compressed_size += sizeof(block_hdr) + res.output_data.size();
        }
    }

    if (in_size > 0) {
        munmap(in_map, in_size);
    }
    close(fd_in);

    const auto end_time = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<double> elapsed = end_time - start_time;
    const double throughput = (in_size / (1024.0 * 1024.0)) / elapsed.count();

    if (print_info) {
        std::println("[+] Compressed {} bytes to {} bytes", in_size, total_compressed_size + sizeof(header));
        std::println("[+] Speed: {:.4f} s | Throughput: {:.2f} MB/s", elapsed.count(), throughput);
    }
}

void Engine::decompress_parallel() const {
    const auto start_time = std::chrono::high_resolution_clock::now();

    int fd_in = open(input_path.c_str(), O_RDONLY);
    if (fd_in < 0) throw std::runtime_error("Cannot open input file.");
    
    struct stat sb;
    if (fstat(fd_in, &sb) < 0) throw std::runtime_error("Cannot stat input file.");
    const size_t in_size = sb.st_size;
    
    uint8_t* in_map = static_cast<uint8_t*>(mmap(nullptr, in_size, PROT_READ, MAP_PRIVATE, fd_in, 0));
    if (in_map == MAP_FAILED) throw std::runtime_error("Failed to mmap input file.");
    
    size_t in_offset = 0;

    ArchiveHeader header;
    std::memcpy(&header, in_map + in_offset, sizeof(ArchiveHeader));
    in_offset += sizeof(ArchiveHeader);
    
    if (std::strncmp(header.magic, "QARC", 4) != 0) throw std::runtime_error("Invalid QARC archive!");

    if (header.codec_id != LZ77::get_codec_id() && header.codec_id != LZ78::get_codec_id()) {
        throw std::runtime_error("Unsupported codec ID in archive: " + std::to_string(header.codec_id));
    }

    int fd_out = open(output_path.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd_out < 0) throw std::runtime_error("Cannot open output file.");
    
    if (ftruncate(fd_out, header.original_size) < 0) throw std::runtime_error("Failed to truncate output file.");
    
    uint8_t* out_map = nullptr;
    if (header.original_size > 0) {
        out_map = static_cast<uint8_t*>(mmap(nullptr, header.original_size, PROT_WRITE, MAP_SHARED, fd_out, 0));
        if (out_map == MAP_FAILED) throw std::runtime_error("Failed to mmap output file.");
    }
    
    size_t out_offset = 0;

    const size_t num_threads = std::thread::hardware_concurrency();
    ThreadPool pool(num_threads, header.codec_id);
    std::queue<std::future<Result>> futures;
    
    const size_t MAX_IN_FLIGHT = num_threads * 2;
    size_t chunk_id = 0;

    if (print_info) {
        std::println("[*] Detected QARC Archive. Dispatching to {} threads...", num_threads);
    }

    while (in_offset < in_size || !futures.empty()) {
        while (in_offset < in_size && futures.size() < MAX_IN_FLIGHT) {
            if (in_offset + sizeof(BlockHeader) > in_size) {
                throw std::runtime_error("Corrupted archive: Incomplete block header.");
            }

            BlockHeader block_hdr;
            std::memcpy(&block_hdr, in_map + in_offset, sizeof(BlockHeader));
            in_offset += sizeof(BlockHeader);

            if (in_offset + block_hdr.compressed_size > in_size) {
                throw std::runtime_error("Corrupted archive: Incomplete compressed block.");
            }

            std::vector<uint8_t> compressed_payload(block_hdr.compressed_size);
            std::memcpy(compressed_payload.data(), in_map + in_offset, block_hdr.compressed_size);
            in_offset += block_hdr.compressed_size;

            futures.push(pool.enqueue_chunk(
                chunk_id++, 
                JobType::Decompress, 
                block_hdr.original_size, 
                std::move(compressed_payload)
            ));
        }

        if (!futures.empty()) {
            Result res = futures.front().get();
            futures.pop();
            
            if (out_offset + res.output_data.size() > header.original_size) {
                throw std::runtime_error("Corrupted archive: Decompressed size exceeds header specification.");
            }
            
            if (out_map) {
                std::memcpy(out_map + out_offset, res.output_data.data(), res.output_data.size());
            }
            out_offset += res.output_data.size();
        }
    }

    munmap(in_map, in_size);
    close(fd_in);
    
    if (out_map) {
        msync(out_map, header.original_size, MS_SYNC);
        munmap(out_map, header.original_size);
    }
    close(fd_out);

    const auto end_time = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<double> elapsed = end_time - start_time;
    const double throughput = (out_offset / (1024.0 * 1024.0)) / elapsed.count();

    if (print_info) {
        std::println("[+] Successfully extracted {} bytes.", out_offset);
        std::println("[+] Speed: {:.4f} s | Throughput: {:.2f} MB/s", elapsed.count(), throughput);
    }
}

void Engine::print_usage() const {
    std::println("Usage:\n"
                 "  Compress (LZ77): ./quant_archiver -lz77 <input_file> <output_archive>\n"
                 "  Compress (LZ78): ./quant_archiver -lz78 <input_file> <output_archive>\n"
                 "  Extract:         ./quant_archiver -x <input_archive> <output_file>");
}
