// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <iostream>
#include <thread>
#include <array>

#include "settings.h"
#include "th_queue.h"
#include "net.h"

point char_to_point(const char* src) {
    float parts[3];
    conv_t<float> conv;
    size_t offset = 0;

    for (int i = 0; i < 3; i++) {
        std::memcpy(conv.side1, &src[offset], sizeof(float));
        offset+=sizeof(float);
        parts[i] = conv.side2;
    }
    return { parts[0], parts[1], parts[2] };
}

char* point_to_char(const point& p) {
    char* ret = new char[sizeof(point)];
    conv_t<float> conv;
    size_t offset = 0;

    conv.side2 = p.x;
    std::memcpy(&ret[offset], conv.side1, sizeof(float));
    offset+=sizeof(float);

    conv.side2 = p.y;
    std::memcpy(&ret[offset], conv.side1, sizeof(float));
    offset+=sizeof(float);

    conv.side2 = p.z;
    std::memcpy(&ret[offset], conv.side1, sizeof(float));

    return ret;
}

char* triangle_to_char(const triangle& t) {
    char* ret = new char[sizeof(triangle)];
    size_t offset = 0;

    std::memcpy(&ret[offset], point_to_char(t.A), sizeof(point));
    offset+=sizeof(point);
    std::memcpy(&ret[offset], point_to_char(t.B), sizeof(point));
    offset+=sizeof(point);
    std::memcpy(&ret[offset], point_to_char(t.C), sizeof(point));

    return ret;
}

char* vec3_to_char(const vec3& v) {
    char* ret = new char[sizeof(vec3)];
    size_t offset = 0;

    std::memcpy(&ret[offset], point_to_char(v.from), sizeof(point));
    offset+=sizeof(point);
    std::memcpy(&ret[offset], point_to_char(v.to), sizeof(point));

    return ret;
}

unsigned int port_server = 3700;

reader_network::reader_network(socket_int_t s) : reader_base() {
    std::cout << "Connected to: " << master_addr << std::endl;
    conn = s;
}

reader_network::~reader_network() {}

bool reader_network::get_next_triangle(triangle* ret) {
    if (!server_have_triangles) return false;
    net_msg ans;
    socket_send_msg(conn, msg_types::CLIENT_GET_TRIANGLE);

    bool ok = socket_get_msg(conn, &ans);
    if (!ok || ans.type != msg_types::SERVER_DATA) {
        server_have_triangles = false;
        return false;
    }
    if (ans.len < sizeof(triangle)) return false;

    // Переводим массив char в triangle
    size_t offset = 0;
    ret->A = char_to_point(&ans.data[offset]);
    offset+=sizeof(point);

    ret->B = char_to_point(&ans.data[offset]);
    offset+=sizeof(point);

    ret->C = char_to_point(&ans.data[offset]);

    return true;
}

void reader_network::get_vectors() {
    net_msg ans;
    socket_send_msg(conn, msg_types::CLIENT_GET_VECTORS);

    bool ok = socket_get_msg(conn, &ans);
    if (!ok || ans.type != msg_types::SERVER_DATA) return;
    vectors.clear();

    size_t offset = 0;
    while (offset < ans.len) {
        auto from = char_to_point(&ans.data[offset]);
        offset+=sizeof(point);
        auto to = char_to_point(&ans.data[offset]);
        offset+=sizeof(point);

        vectors.push_back({from, to});
    }
}

bool reader_network::have_triangles() const {
    return server_have_triangles;
}

saver_network::saver_network(socket_int_t s) : saver_base() {
    conn = s;
    file.open("output.tmp", std::ios::out | std::ios::app | std::ios::ate | std::ios::binary);
    if (!file.good()) throw std::runtime_error("Failed to open output.tmp");
}

saver_network::~saver_network() {
    file.close();
}

void saver_network::save_tmp(volatile char** mat, const unsigned int count) {
    const size_t vec_count = vectors.size();
	for (size_t vec = 0; vec < vec_count; vec++) {
		auto curr_vec = vectors[vec];
		curr_vec.from.print_terse(file); file << ">";
		curr_vec.to.print_terse(file); file << ":";

		for (size_t tr = 0; tr < count; tr++)
			if (mat[vec][tr] == 1) {
				auto t = triangles[tr];
				t.A.print_terse(file); file << " ";
				t.B.print_terse(file); file << " ";
				t.C.print_terse(file); file << " ";
			}
		file << '\n';
	}
	file.flush();
}

void saver_network::save_final() {
    // Загружаем output.tmp в память
    std::vector<char> buf;
    file.seekg(0);
    while (!file.eof()) buf.push_back(file.get());

    // Собираем сообщение
    net_msg msg;
    msg.type = msg_types::CLIENT_DATA;
    msg.len = buf.size();
    msg.data = new char[msg.len];
    for (size_t i = 0; i < msg.len; i++) msg.data[i] = buf[i];

    socket_send_msg(conn, msg);
}

void master_start(socket_int_t socket) {
    std::cout << "Master mode" << std::endl;
    std::cout << "Listen port " << port_server << std::endl;
    auto parser = select_parser(nullptr);
    std::fstream tmpfile(output_file.string() + ".tmp", std::ios::app | std::ios::ate | std::ios::binary);

    // Загружаем вектора
    parser->get_vectors();
    const auto vectors_array_size = vectors.size() * sizeof(vec3);
    char* vectors_array = new char[vectors_array_size];
    size_t offset = 0;
    for (auto& v : vectors) {
        auto vec3_raw = vec3_to_char(v);
        std::memcpy(&vectors_array[offset], vec3_raw, sizeof(vec3));
        delete[] vec3_raw;
        offset+=sizeof(vec3);
    }

    // Открываем поток приема сообщений
    th_queue<net_msg> queue;
    volatile bool threads_run = true;
    socket_int_t** clients = new socket_int_t*[clients_max];
    for (size_t i = 0; i < clients_max; i++) clients[i] = nullptr;
    std::thread netd(netd_server, socket, clients, &queue, &threads_run);
    while (!netd_started) continue;

    std::cout << "Ready." << std::endl;
    while (run_server()) {
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
                net_msg ans;
                ans.type = msg_types::SERVER_DATA;
                ans.len = sizeof(triangle);
                ans.data = triangle_to_char(ret);

                socket_send_msg(send_to, ans);
                break;
            }
            case msg_types::CLIENT_DATA: {
                std::cout << "Client: local results saved" << std::endl;
                tmpfile << msg.data << std::endl;
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
}