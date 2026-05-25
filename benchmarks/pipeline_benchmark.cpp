#include <benchmark/benchmark.h>
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>
#include <cstdlib>
#include <cstring>

#include "Engine.h"

namespace fs = std::filesystem;

std::vector<uint8_t> load_test_file(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file) {
        throw std::runtime_error("Could not open file: " + filepath);
    }
    
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<uint8_t> buffer(size);
    file.read(reinterpret_cast<char*>(buffer.data()), size);
    return buffer;
}

std::string create_temp_file() {
    char temp_path[] = "/tmp/compress_bench_XXXXXX";
    int fd = mkstemp(temp_path);
    if (fd == -1) {
        throw std::runtime_error("Failed to create temp file");
    }
    close(fd);
    return std::string(temp_path);
}

static void BM_Engine_Compress_Shakespeare(benchmark::State& state) {
    auto data = load_test_file("test_data/Shakespeare.txt");
    
    for (auto _ : state) {
        state.PauseTiming();

        std::string input_file = create_temp_file();
        std::string output_file = create_temp_file();
        
        std::ofstream out(input_file, std::ios::binary);
        out.write(reinterpret_cast<const char*>(data.data()), data.size());

        state.ResumeTiming();
    
        Engine engine("-lz77", input_file, output_file, false);
        engine.run();

        state.PauseTiming();
        fs::remove(input_file);
        fs::remove(output_file);
        state.ResumeTiming();
    }
    
    state.counters["bytes_processed"] = benchmark::Counter(
        data.size(), benchmark::Counter::kIsIterationInvariant);
}

static void BM_Engine_Decompress_Shakespeare(benchmark::State& state) {
    auto data = load_test_file("test_data/Shakespeare.txt");
    
    for (auto _ : state) {
        state.PauseTiming();
        
        std::string input_file = create_temp_file();
        std::string compressed_file = create_temp_file();
        std::string decompressed_file = create_temp_file();
        
        std::ofstream out(input_file, std::ios::binary);
        out.write(reinterpret_cast<const char*>(data.data()), data.size());
        out.close();
        
        Engine comp_engine("-lz77", input_file, compressed_file, false);
        comp_engine.run();
        
        state.ResumeTiming();

        Engine engine("-x", compressed_file, decompressed_file, false);
        engine.run();

        state.PauseTiming();
        fs::remove(input_file);
        fs::remove(compressed_file);
        fs::remove(decompressed_file);
        state.ResumeTiming();
    }
    
    state.counters["bytes_processed"] = benchmark::Counter(
        data.size(), benchmark::Counter::kIsIterationInvariant);
}

BENCHMARK(BM_Engine_Compress_Shakespeare);
BENCHMARK(BM_Engine_Decompress_Shakespeare);

BENCHMARK_MAIN();
