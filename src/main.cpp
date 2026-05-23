#include <string>
#include <print>

#include "LZ77.h"
#include "Engine.h"

int main(int argc, char* argv[]) {
    if (argc != 4) {
        Engine engine;
        engine.print_usage();
        return 1;
    }

    std::string mode = argv[1];
    std::string input_path = argv[2];
    std::string output_path = argv[3];

    try {
        Engine engine(mode, input_path, output_path);
        engine.run();
    } catch (const std::exception& e) {
        std::println("[!] Error: {}", e.what());
        return 1;
    }

    return 0;
}
