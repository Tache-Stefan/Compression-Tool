#include <benchmark/benchmark.h>
#include <fstream>
#include <vector>
#include <string>

#include "LZ77.h"

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

static void BM_LZ77_Compress_Shakespeare(benchmark::State& state) {
    auto data = load_test_file("test_data/Shakespeare.txt");
    
    for (auto _ : state) {
        LZ77 engine;
        auto compressed = engine.compress_block(data);
        benchmark::DoNotOptimize(compressed);
    }
    
    state.counters["bytes_processed"] = benchmark::Counter(
        data.size(), benchmark::Counter::kIsIterationInvariant);
}

static void BM_LZ77_Compress_Moby_Dick(benchmark::State& state) {
    auto data = load_test_file("test_data/Moby_Dick.txt");
    
    for (auto _ : state) {
        LZ77 engine;
        auto compressed = engine.compress_block(data);
        benchmark::DoNotOptimize(compressed);
    }
    
    state.counters["bytes_processed"] = benchmark::Counter(
        data.size(), benchmark::Counter::kIsIterationInvariant);
}

static void BM_LZ77_Decompress_Shakespeare(benchmark::State& state) {
    auto original_data = load_test_file("test_data/Shakespeare.txt");
    LZ77 compressor;
    auto compressed = compressor.compress_block(original_data);
    
    for (auto _ : state) {
        LZ77 engine;
        auto decompressed = engine.decompress_block(compressed, original_data.size());
        benchmark::DoNotOptimize(decompressed);
    }
    
    state.counters["bytes_processed"] = benchmark::Counter(
        original_data.size(), benchmark::Counter::kIsIterationInvariant);
}

static void BM_LZ77_Decompress_Moby_Dick(benchmark::State& state) {
    auto original_data = load_test_file("test_data/Moby_Dick.txt");
    LZ77 compressor;
    auto compressed = compressor.compress_block(original_data);
    
    for (auto _ : state) {
        LZ77 engine;
        auto decompressed = engine.decompress_block(compressed, original_data.size());
        benchmark::DoNotOptimize(decompressed);
    }
    
    state.counters["bytes_processed"] = benchmark::Counter(
        original_data.size(), benchmark::Counter::kIsIterationInvariant);
}

BENCHMARK(BM_LZ77_Compress_Shakespeare);
BENCHMARK(BM_LZ77_Compress_Moby_Dick);
BENCHMARK(BM_LZ77_Decompress_Shakespeare);
BENCHMARK(BM_LZ77_Decompress_Moby_Dick);

BENCHMARK_MAIN();
