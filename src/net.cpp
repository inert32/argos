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

reader_network::reader_network(socket_t* s) : reader_base() {
    std::cout << "Connected to: " << master_addr << std::endl;
    conn = s;
}

reader_network::~reader_network() {
    net_msg bye;
    bye.type = msg_types::CLIENT_DISCONNECT;

    conn->send_msg(bye);
    conn = nullptr;
}

bool reader_network::get_next_triangle(triangle* ret) {
    std::cout << "reader_network: request triangle" << std::endl;
    if (!server_have_triangles) return false;
    net_msg ans;
    conn->send_msg(msg_types::CLIENT_GET_TRIANGLE);

    bool ok = conn->get_msg(&ans);
    std::cout << "Server reply: " << (int)ans.type << ", need " << (int)msg_types::SERVER_DATA << std::endl;
    if (!ok || ans.type != msg_types::SERVER_DATA) {
        server_have_triangles = false;
        return false;
    }
    if (ans.len != sizeof(triangle)) return false;

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
    std::cout << "reader_network: request vectors" << std::endl;
    net_msg ans;
    conn->send_msg(msg_types::CLIENT_GET_VECTORS);

    bool ok = conn->get_msg(&ans);
    std::cout << "Server reply: " << (int)ans.type << ", need " << (int)msg_types::SERVER_DATA << std::endl;
    if (!ok || ans.type != msg_types::SERVER_DATA) {
        return;
    }
    vectors.clear();

    size_t offset = 0, i = 0;
    while (i < ans.len) {
        auto from = char_to_point(&ans.data[offset]);
        offset+=sizeof(point);
        auto to = char_to_point(&ans.data[offset]);
        offset+=sizeof(point);

        vectors.push_back({from, to});
    }
}

bool reader_network::have_triangles() const {
    return true;
}

saver_network::saver_network(socket_t* s) : saver_base() {
    conn = s;
    file.open("output.tmp", std::ios::out | std::ios::app | std::ios::ate | std::ios::binary);
    if (!file.good()) throw std::runtime_error("Failed to open output.tmp");
}

saver_network::~saver_network() {
    file.close();
    conn = nullptr;
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

    conn->send_msg(msg);
}

void master_start(socket_t* socket) {
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
        offset+=sizeof(vec3);
        delete[] vec3_raw;
    }

    // Открываем поток приема сообщений
    for (size_t i = 0; i < clients_max; i++) clients_list[i] = nullptr;

    th_queue<net_msg> queue;
    volatile bool threads_run = true;

    //netd_server(socket, clients_list, &queue, &threads_run);

    std::thread netd(netd_server, socket, &queue, &threads_run);
    while (!netd_started) continue;

    bool wait_for_clients = true;
    std::cout << "Ready." << std::endl;
    while (wait_for_clients) {
        auto x = queue.take();
        if (!x) continue;
        auto msg = *x;
        auto send_to = *clients_list[msg.peer_id];
        std::cout << "Type: " << (int)msg.type << std::endl;
        std::cout << "Len: " << msg.len << std::endl;
        bool ok;
        switch (msg.type) {
            case msg_types::CLIENT_GET_VECTORS: {
                std::cout << "request vectors" << std::endl;
                net_msg ans;
                ans.type = msg_types::SERVER_DATA;
                ans.data = vectors_array;
                ans.len = vectors_array_size;
                send_to.send_msg(ans);

                ok = send_to.send_msg(ans);
                if (ok) std::cout << "send ok" << std::endl;
                else std::cout << "send fail" << std::endl;
                break;
            }
            case msg_types::CLIENT_GET_TRIANGLE: {
                std::cout << "request triangle" << std::endl;
                triangle ret;
                ok = parser->get_next_triangle(&ret);
                if (!ok) {
                    send_to.send_msg(msg_types::SERVER_DATA);
                    break;
                }
                net_msg ans;
                ans.type = msg_types::SERVER_DATA;
                ans.len = sizeof(triangle);

                /*ans.data = new char[sizeof(triangle)];
                auto triangle_raw = triangle_to_char(ret);
                std::memcpy(ans.data, triangle_raw, sizeof(triangle));
                delete[] triangle_raw;*/
                ans.data = triangle_to_char(ret);

                ok = send_to.send_msg(ans);
                if (ok) std::cout << "send ok" << std::endl;
                else std::cout << "send fail" << std::endl;
                break;
            }
            case msg_types::CLIENT_DATA: {
                std::cout << "request save" << std::endl;
                tmpfile << msg.data << std::endl;
                break;
            }
            case msg_types::CLIENT_DISCONNECT: {
                std::cout << "client disconnect" << std::endl;
                clients_list[msg.peer_id]->~socket_t();
                clients_list[msg.peer_id] = nullptr;
                break;
            }
            default: {
                send_to.send_msg(msg_types::BOTH_UNKNOWN);
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