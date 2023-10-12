// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <iostream>
#include <filesystem>
#include <string>
#include <thread>

#include "common.h"
#include "solo.h"

std::filesystem::path verticies_file;
std::filesystem::path output_file = "output.list";
unsigned int ipv4addr = (unsigned)-1;
bool master_mode = false;
unsigned int chunk_elements = 100;
unsigned int threads_count = std::thread::hardware_concurrency();

void client_start() {}
void master_start() {}

int show_help() {
    std::cout << "usage: argos --file <PATH> | --connect <ADDR>" << std::endl;
    std::cout << "     --file <PATH>     - use verticies file" << std::endl;
    std::cout << "(N/A)--connect <ADDR>  - connect to master server (IPv4 only)" << std::endl;
    std::cout << "(N/A)--master          - be master node" << std::endl;
    std::cout << "     --output <PATH>   - path to the output file (default " << output_file << ")" << std::endl;
    std::cout << "     --chunk <COUNT>   - count of triangles to load per cycle (default " << chunk_elements << ")" << std::endl;
    std::cout << "     --threads <COUNT> - count of threads for calculation (default " << threads_count << ")" << std::endl;
    std::cout << "     --help            - this help" << std::endl;
    return 0;
}

bool parse_cli(int argc, char** argv) {
    if (argc == 1 || std::string(argv[1]) == "--help") return false;
    for (int i = 1; i < argc; i++) {
        std::string buf(argv[i]);

        if (buf == "--file") {
            if (i + 1 < argc) verticies_file = argv[i + 1];
            else {
                std::cerr << "err: parse_cli: no verticies file provided." << std::endl;
                return false;
            }
            i++;
        }
        if (buf == "--output") {
            if (i + 1 < argc) output_file = argv[i + 1];
            else std::cerr << "warn: parse_cli: no output file provided, using default: " << output_file << std::endl;
            i++;
        }
        if (buf == "--chunk") {
            try {
                chunk_elements = std::stoi(argv[i + 1]);
            }
            catch (const std::exception&) {
                std::cerr << "warn: parse_cli: no chunk count provided, using default: " << chunk_elements << std::endl;
            }
            i++;
        }
        if (buf == "--threads") {
            try {
                threads_count = std::stoi(argv[i + 1]);
            }
            catch (const std::exception&) {
                std::cerr << "warn: parse_cli: no threads count provided, using default: " << threads_count << std::endl;
            }
            if (threads_count > std::thread::hardware_concurrency())
                std::cerr << "warn: parse_cli: requested threads count exceed native (" 
                << std::thread::hardware_concurrency() << ")" << std::endl;
            if (threads_count == 0) threads_count = 1;
            i++;
        }
    }
    
    return true;
}

int main(int argc, char** argv) {
    std::cout << "argos " << ARGOS_VERSION << std::endl;
    if (!parse_cli(argc, argv)) return show_help();

    // Очищаем пути к файлам
    if (!verticies_file.empty()) verticies_file = std::filesystem::absolute(verticies_file);
    if (!output_file.empty()) output_file = std::filesystem::absolute(output_file);

    // Определяем режим работы
    if (ipv4addr == (unsigned)-1 && verticies_file.empty()) {
        std::cerr << "err: main: no verticies file and no master server address provided." << std::endl;
        return 1;
    }

    if (ipv4addr != (unsigned)-1) client_start();
    else {
        if (master_mode) master_start();
        else solo_start();
    }

    return 0;
}