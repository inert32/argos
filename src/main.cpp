// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <iostream>
#include <filesystem>
#include <string>
#include <thread>

#include "settings.h"
#include "base.h"
#include "net.h"

std::filesystem::path verticies_file;
std::filesystem::path output_file = "output.list";
bool master_mode = false;
unsigned int chunk_elements = 100;
unsigned int threads_count = 0;
ipv4_t master_addr;

void solo_start(socket_int* socket);

int show_help() {
    std::cout << "usage: argos --file <PATH> | --connect <ADDR>" << std::endl;
    std::cout << "     --file <PATH>          - use verticies file" << std::endl;
    std::cout << "(N/A)--connect <ADDR:port>  - connect to master server (IPv4 only)" << std::endl;
    std::cout << "(N/A)--master               - be master node" << std::endl;
    std::cout << "     --output <PATH>        - path to the output file (default " << output_file << ")" << std::endl;
    std::cout << "     --chunk <COUNT>        - count of triangles to load per cycle (default " << chunk_elements << ")" << std::endl;
    std::cout << "     --threads <COUNT>      - count of threads for calculation (default " << threads_count << ")" << std::endl;
    std::cout << "     --help                 - this help" << std::endl;
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

    if (!master_addr.is_set() && verticies_file.empty()) {
        std::cerr << "err: main: no verticies file and no master server address provided." << std::endl;
        return 1;
    }

    // Определяем режим работы
    if (master_mode) { // Режим мастера
        auto socket = new socket_int(3456);
        master_start(socket);
        delete socket;
    }
    else {
        if (master_addr.is_set()) { // Режим клиента
            auto socket = new socket_int(3700);
            solo_start(socket);
            delete socket;
        }
        else solo_start(nullptr); // Одиночный режим
    }

    return 0;
}

reader_base* select_parser([[maybe_unused]] socket_int* socket) {
	if (socket != nullptr) return new reader_network(socket);

	std::ifstream file(verticies_file, std::ios::binary);
	if (!file.good()) throw std::runtime_error("select_parser: Failed to open file " + output_file.string());

	// Проверяем заголовки
	std::string header;
	std::getline(file, header);
	file.close();

	if (header[0] == 'V' && header[1] == ':') // Старый формат
		return new reader_argos();
	else // Неизвестный формат 
		throw std::runtime_error("select_parser: " + verticies_file.string() + ": unknown format.");
}

saver_base* select_saver([[maybe_unused]] socket_int* socket) {
	if (socket != nullptr) return new saver_network(socket);

	return new saver_file;
}