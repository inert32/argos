// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <iostream>
#include <filesystem>
#include <string>
#include <thread>

#include "settings.h"
#include "base.h"
#include "net.h"

// Начало работы в одиночном режиме
void solo_start(socket_int_t* socket);

reader_base* select_parser(socket_int_t* s) {
    if (s != nullptr) return new reader_network(*s);

    std::ifstream file(verticies_file, std::ios::binary);
    if (!file.good()) throw std::runtime_error("select_parser: Failed to open file " + output_file.string());

    // Проверяем заголовки
    std::string header;
    std::getline(file, header);
    file.close();

    if (header[0] == 'V' && header[1] == ':')
        return new reader_argos();
    else // Неизвестный формат
        throw std::runtime_error("select_parser: " + verticies_file.string() + ": unknown format.");
}

saver_base* select_saver(socket_int_t* s) {
    if (s != nullptr) return new saver_network(*s);

    return new saver_local();
}

int show_help() {
    std::cout << "usage: argos --file <PATH> | --connect <ADDR>" << std::endl;
    std::cout << "     --file <PATH>         - use verticies file" << std::endl;
    std::cout << "     --connect <ADDR>      - connect to master server (IPv4 only)" << std::endl;
    std::cout << "     --master              - be master node (default off)" << std::endl;
    std::cout << "     --output <PATH>       - path to the output file (default " << output_file << ")" << std::endl;
    std::cout << "     --chunk <COUNT>       - count of triangles to load per cycle (default " << chunk_elements << ")" << std::endl;
    std::cout << "     --threads <COUNT>     - count of threads for calculation (default " << threads_count << ")" << std::endl;
    std::cout << "     --port <PORT>         - set server port (default " << port_server << ")" << std::endl;
    std::cout << "     --min-clients <COUNT> - set minimal clients count to start (default " << clients_min << ")" << std::endl;
    std::cout << "     --max-clients <COUNT> - set maximum clients count (default " << clients_max << ")" << std::endl;
    std::cout << "     --keep-tmp            - keep temporary files (default off)";
    std::cout << "     --help                - this help" << std::endl;
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

        if (buf == "--keep-tmp") keep_tmp = true;
        if (buf == "--master") master_mode = true;
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
        if (i + 1 < argc) {
            i++;
            if (buf == "--chunk") {
                try { chunk_elements = std::stoi(argv[i]); }
                catch (const std::exception&) { chunk_elements = 100; }
            }
            if (buf == "--threads") {
                try { threads_count = std::stoi(argv[i]); }
                catch (const std::exception&) { threads_count = 0; }
            }
            if (buf == "--connect") master_addr.from_string(argv[i]);
            if (buf == "--port") {
                try { port_server = std::stoi(argv[i]); }
                catch (const std::exception&) { port_server = 3700; }
            }
            if (buf == "--min-clients") {
                try { clients_min = std::stoul(argv[i]); }
                catch (const std::exception&) { clients_min = 0; }
            }
            if (buf == "--max-clients") {
                try { clients_max = std::stoul(argv[i]); }
                catch (const std::exception&) { clients_max = 10; }
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

    if (verticies_file.empty() && !master_addr) {
        std::cerr << "err: main: no verticies file and no master server address provided." << std::endl;
        return 1;
    }

    // Определяем режим работы
    try {
        init_network();
        if (master_mode) { // Режим мастер-сервера
            socket_int_t sock = socket_setup();
            master_start(sock);
            socket_close(sock);
        }
        else {
            threads_count_setup();
            std::cout << "Using " << threads_count << " worker threads." << std::endl;

            if (master_addr) { // Режим клиента
                socket_int_t sock = socket_setup();
                solo_start(&sock);
                socket_send_msg(sock, msg_types::CLIENT_DISCONNECT);
                socket_close(sock);
            }
            else solo_start(nullptr); // Одиночный режим
        }
        shutdown_network();
    }
    catch (const std::exception& e) {
        std::cerr << "err: " << e.what() << std::endl;
        shutdown_network();
    }

    return 0;
}
