// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <iostream>
#include <filesystem>
#include <string>
#include <thread>

#include "settings.h"
#include "base.h"
#include "solo.h"
#include "net.h"

std::filesystem::path verticies_file;
std::filesystem::path output_file = "output.list";
ipv4_t master_addr;
bool master_mode = false;
unsigned int chunk_elements = 100;
unsigned int threads_count = 0;

// Начало работы в одиночном режиме
void solo_start(socket_t* socket);

int show_help() {
    std::cout << "usage: argos --file <PATH> | --connect <ADDR>" << std::endl;
    std::cout << "     --file <PATH>     - use verticies file" << std::endl;
    std::cout << "     --connect <ADDR>  - connect to master server (IPv4 only)" << std::endl;
    std::cout << "     --master          - be master node" << std::endl;
    std::cout << "     --output <PATH>   - path to the output file (default " << output_file << ")" << std::endl;
    std::cout << "     --chunk <COUNT>   - count of triangles to load per cycle (default " << chunk_elements << ")" << std::endl;
    std::cout << "     --threads <COUNT> - count of threads for calculation (default " << threads_count << ")" << std::endl;
    std::cout << "     --port <PORT>     - set server port (default " << port_server << ")" << std::endl;
    std::cout << "     --clients <NUM>   - set maximum clients count (default " << clients_max << ")" << std::endl;
    std::cout << "     --help            - this help" << std::endl;
    return 0;
}

// Установка количества потоков
void threads_count_setup() {
    if (threads_count == 0) {
        // Берем только потоки физических процессоров
        threads_count = std::thread::hardware_concurrency() / 2;
        // hardware_concurrency() может не определить количество потоков процессора
        if (threads_count == 0) threads_count = 1;
    }
}

// Установка параметров командной строки
bool parse_cli(int argc, char** argv) {
    if (argc == 1 || std::string(argv[1]) == "--help") return false;
    for (int i = 1; i < argc; i++) {
        std::string buf(argv[i]);

        if (buf == "--file") {
            if (i + 1 < argc) verticies_file = argv[++i];
            else {
                std::cerr << "err: parse_cli: no verticies file provided." << std::endl;
                return false;
            }
        }
        if (buf == "--output") {
            if (i + 1 < argc) output_file = argv[++i];
            else std::cerr << "warn: parse_cli: no output file provided, using default: " << output_file << std::endl;
        }
        if (buf == "--chunk") {
            if (i + 1 < argc) {
                try {
                    chunk_elements = std::stoi(argv[++i]);
                }
                catch (const std::exception&) {
                    std::cerr << "warn: parse_cli: no chunk count provided, using default: " << chunk_elements << std::endl;
                }
            }
        }
        if (buf == "--threads") {
            if (i + 1 < argc) {
                try {
                    threads_count = std::stoi(argv[++i]);
                }
                catch (const std::exception&) {
                    threads_count = 0;
                }
            }
        }
        if (buf == "--master") master_mode = true;
        if (buf == "--connect") if (i + 1 < argc) master_addr.from_string(argv[++i]);
        if (buf == "--port") {
            if (i + 1 < argc) {
                try {
                    port_server = std::stoi(argv[++i]);
                }
                catch (const std::exception&) {
                    port_server = 3700;
                }
            }
        }
        if (buf == "--clients") {
            if (i + 1 < argc) {
                try {
                    clients_max = std::stoi(argv[++i]);
                }
                catch (const std::exception&) {
                    clients_max = 10;
                }
            }
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

    threads_count_setup();
    std::cout << "Using " << threads_count << " worker threads." << std::endl;

    if (verticies_file.empty() && !master_addr) {
        std::cerr << "err: main: no verticies file and no master server address provided." << std::endl;
        return 1;
    }

    // Определяем режим работы
    try {
        socket_t sock;

        if (master_mode) master_start(&sock); // Ражим мастера
        else if (master_addr) solo_start(&sock); // Режим клиента
        else solo_start(nullptr); // Одиночный режим
    }
    catch (const std::exception& e) {
        std::cerr << "err: " << e.what() << std::endl;
    }

    return 0;
}