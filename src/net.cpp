// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <iostream>
#include <thread>
#include <array>

#include "settings.h"
#include "th_queue.h"
#include "net.h"

unsigned int port_server = 3700;

// Функции конвертации

reader_network::reader_network(socket_int_t s) : reader_base() {
    std::cout << "Connected to: " << master_addr << std::endl;
    conn = s;
}

bool reader_network::get_next_triangle(triangle* ret) {
    if (!server_have_triangles) return false;
    net_msg ans;
    socket_send_msg(conn, msg_types::CLIENT_GET_TRIANGLE);

    bool ok = socket_get_msg(conn, &ans);
    if (!ok || ans.type != msg_types::SERVER_DATA) {
        server_have_triangles = false;
        return false;
    }
    if (ans.len < triangle::size) return false; // Данные неполны

    // Переводим массив char в triangle
    size_t offset = 0;
    ret->A.from_char(&ans.data[offset]);
    offset+=sizeof(point);

    ret->B.from_char(&ans.data[offset]);
    offset+=sizeof(point);

    ret->C.from_char(&ans.data[offset]);

    return true;
}

// Клиент не должен просить определенные треугольники, это необходимо
// только на этапе saver_base::convert_ids()
bool reader_network::get_triangle([[maybe_unused]] triangle* ret, [[maybe_unused]] const size_t id) {
    return false;
}

void reader_network::get_vectors() {
    net_msg ans;
    socket_send_msg(conn, msg_types::CLIENT_GET_VECTORS);

    bool ok = socket_get_msg(conn, &ans);
    if (!ok || ans.type != msg_types::SERVER_DATA) return; // Получен большой массив векторов
    vectors.clear();

    size_t offset = 0;
    size_t id = 0;
    while (offset < ans.len) {
        point from(&ans.data[offset]);
        offset+=sizeof(point);
        point to(&ans.data[offset]);
        offset+=sizeof(point);

        vectors.push_back({id++, from, to});
    }
    delete[] ans.data;
}

bool reader_network::have_triangles() const {
    return server_have_triangles;
}

saver_network::saver_network(socket_int_t s) : saver_base() {
    conn = s;
}

void saver_network::save_tmp(volatile char** mat, const unsigned int count) {
	const size_t vec_count = vectors.size();
    std::string payload;
	// Для каждого вектора указываем
	for (size_t vec = 0; vec < vec_count; vec++) {
		auto& curr_vec = vectors[vec];
		payload += std::to_string(curr_vec.id) + ":";

		for (size_t tr = 0; tr < count; tr++)
			if (mat[vec][tr] == 1) {
				auto& t = triangles[tr];
                payload += std::to_string(t.id) + " ";
			}
		payload += '\n';
	}

    net_msg msg;
    msg.type = msg_types::CLIENT_DATA;
    msg.len = payload.length();
    msg.data = new char[msg.len];
    std::memcpy(msg.data, payload.c_str(), payload.length());

    socket_send_msg(conn, msg);
    delete[] msg.data;
}

void master_start(socket_int_t socket) {
    std::cout << "Master mode" << std::endl;
    std::cout << "Listen port " << port_server << std::endl;
    auto parser = select_parser(nullptr);
    std::fstream tmpfile(output_file.string() + ".tmp", std::ios::app | std::ios::ate | std::ios::binary);

    // Загружаем вектора
    parser->get_vectors();
    const auto vectors_array_size = vectors.size() * vec3::size;
    char* vectors_array = new char[vectors_array_size];
    size_t offset = 0;
    for (auto& v : vectors) {
        auto vec3_raw = v.to_char();
        std::memcpy(&vectors_array[offset], vec3_raw, vec3::size);
        delete[] vec3_raw;
        offset+=vec3::size;
    }

    // Открываем поток приема сообщений
    th_queue<net_msg> queue;
    volatile bool threads_run = true;
    socket_int_t** clients = new socket_int_t*[clients_max];
    for (size_t i = 0; i < clients_max; i++) clients[i] = nullptr;
    std::thread netd(netd_server, socket, clients, &queue, &threads_run);
    while (!netd_started) continue;

    std::cout << "Ready." << std::endl;
    size_t triangle_id = 0;
    while (run_server()) { // Обработка сообщений
        auto x = queue.take();
        if (!x) continue;
        auto msg = *x;
        if (clients[msg.peer_id] == nullptr) continue;
        auto send_to = *clients[msg.peer_id];

        switch (msg.type) {
            case msg_types::CLIENT_GET_VECTORS: {
                net_msg ans;
                ans.type = msg_types::SERVER_DATA;
                ans.data = vectors_array;
                ans.len = vectors_array_size;

                socket_send_msg(send_to, ans);
                break;
            }
            case msg_types::CLIENT_GET_TRIANGLE: {
                triangle ret;
                bool ok = parser->get_next_triangle(&ret);
                if (!ok) {
                    socket_send_msg(send_to, msg_types::SERVER_DATA);
                    break;
                }
                ret.id = triangle_id++;
                net_msg ans;
                ans.type = msg_types::SERVER_DATA;
                ans.len = triangle::size;
                ans.data = ret.to_char();

                socket_send_msg(send_to, ans);
                break;
            }
            case msg_types::CLIENT_DATA: {
                for (size_t i = 0; i < msg.len; i++) tmpfile << msg.data[i];
                tmpfile.flush();
                break;
            }
            case msg_types::CLIENT_DISCONNECT: {
                socket_close(*clients[msg.peer_id]);
                clients[msg.peer_id] = nullptr;
                clients_now--;
                break;
            }
            default: {
                socket_send_msg(send_to, msg_types::BOTH_UNKNOWN);
                break;
            }
        }
    }
    threads_run = false;
    while (netd_started) continue;
    netd.join();

    // Сохраняем файл
    tmpfile.close();
    auto saver = select_saver(nullptr);
    saver->save_final();
    saver->convert_ids();
}