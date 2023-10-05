// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <iostream>
#include <filesystem>
#include <string>

#include "common.h"
#include "solo.h"

std::filesystem::path verticies_file;
unsigned int ipv4addr = (unsigned)-1;
bool master_mode = false;
unsigned int chunk_elements = 100;

void client_start() {}
void master_start() {}

int show_help() {
    std::cout << "usage: argos --file <PATH> | --connect <ADDR>" << std::endl;
    std::cout << "       --file <PATH>       - use verticies file" << std::endl;
    std::cout << "(N/A)  --connect <ADDR>    - connect to master server (IPv4 only)" << std::endl;
    std::cout << "(N/A)  --master            - be master node" << std::endl;
    return 0;
}

bool parse_cli(int argc, char** argv) {
    if (argc == 1) return false;
    for (int i = 1; i < argc; i++) {
        std::string buf(argv[i]);

        if (buf == "--file" && i + 1 < argc) verticies_file = argv[++i];
        else {
            std::cerr << "err: parse_cli: no verticies file provided." << std::endl;
            return false;
        }
    }
    
    return true;
}

int main(int argc, char** argv) {
    std::cout << "argos " << ARGOS_VERSION << std::endl;
    if (!parse_cli(argc, argv)) return show_help();

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