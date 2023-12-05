// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <iostream>
#include <thread>
#include <map>

#include "settings.h"
#include "net.h"

reader_network::reader_network(socket_t* s) : reader_base() {
    std::cout << "Connected to: " << master_addr << std::endl;
    conn = s;
}

bool reader_network::get_next_triangle(triangle* ret) {
    if (!server_have_triangles) return false;
    net_msg ans;
    conn->send_msg(msg_types::CLIENT_GET_TRIANGLE);

    bool ok = conn->get_msg(&ans);
    if (!ok || ans.type != msg_types::SERVER_DATA) {
        server_have_triangles = false;
        return false;
    }
    if (ans.len < sizeof(triangle)) return false; // Данные неполны

    // Переводим массив char в triangle
    ret->from_char(ans.data);
    return true;
}

// Клиент не должен просить определенные треугольники,
// это необходимо только saver_base::finalize()
bool reader_network::get_triangle([[maybe_unused]] triangle* ret, [[maybe_unused]] const size_t id) {
    return false;
}

void reader_network::get_vectors() {
    net_msg ans;
    conn->send_msg(msg_types::CLIENT_GET_VECTORS);

    bool ok = conn->get_msg(&ans);
    if (!ok || ans.type != msg_types::SERVER_DATA) return;
    vectors.clear();

    size_t offset = 0;
    while (offset < ans.len) {
        vectors.emplace_back(&ans.data[offset]);
        offset+=sizeof(vec3);
    }
    delete[] ans.data;
}

bool reader_network::have_triangles() const {
    return server_have_triangles;
}

saver_network::saver_network(socket_t* s) : saver_base() {
    conn = s;
}

void saver_network::finalize(const full_map& m) {
    for (auto& i : m) {
        if (i.second.size() == 0) continue;

        net_msg msg;
        msg.type = msg_types::CLIENT_DATA;
        msg.len = sizeof(size_t) * (1 + i.second.size());
        msg.data = new char[msg.len];

        conv_t<size_t> conv;
        conv.side2 = i.first;
        std::memcpy(msg.data, conv.side1, sizeof(size_t));

        size_t offset = sizeof(size_t);
        for (auto& t : i.second) {
            conv.side2 = t;
            std::memcpy(&msg.data[offset], conv.side1, sizeof(size_t));
            offset+=sizeof(size_t);
        }
        conn->send_msg(msg);
    }
}

void master_start(socket_t* socket) {
    std::cout << "Master mode" << std::endl;
    std::cout << "Listen port " << port_server << std::endl;
    auto parser = select_parser(nullptr);

    // Загружаем вектора
    parser->get_vectors();
    const auto vectors_array_size = vectors.size() * sizeof(vec3);
    char* vectors_array = new char[vectors_array_size];
    size_t offset = 0;
    for (auto& v : vectors) {
        std::memcpy(&vectors_array[offset], v.to_char(), sizeof(vec3));
        offset+=sizeof(vec3);
    }
    full_map map;
    for (size_t i = 0; i < vectors.size(); i++) map[i].clear();

    // Открываем поток приема сообщений
    th_queue<net_msg> queue;
    volatile bool threads_run = true;
    clients_list cl;
    std::thread netd(netd_server, socket, &cl, &queue, &threads_run);
    while (!netd_started) continue;

    std::cout << "Ready." << std::endl;
    while (cl.run_server()) { // Обработка сообщений
        auto x = queue.take();
        if (!x) continue;
        auto msg = *x;
        if (cl.get(msg.peer_id) == nullptr) continue;
        auto send_to = *cl.get(msg.peer_id);

        switch (msg.type) {
            case msg_types::CLIENT_GET_VECTORS: {
                net_msg ans;
                ans.type = msg_types::SERVER_DATA;
                ans.data = vectors_array;
                ans.len = vectors_array_size;

                send_to.send_msg(ans);
                break;
            }
            case msg_types::CLIENT_GET_TRIANGLE: {
                triangle ret;
                bool ok = parser->get_next_triangle(&ret);
                if (!ok) {
                    send_to.send_msg(msg_types::SERVER_DATA);
                    break;
                }
                net_msg ans;
                ans.type = msg_types::SERVER_DATA;
                ans.len = sizeof(triangle);
                ans.data = ret.to_char();

                send_to.send_msg(ans);
                break;
            }
            case msg_types::CLIENT_DATA: {
                conv_t<size_t> conv;
                std::memcpy(conv.side1, msg.data, sizeof(size_t));
                const size_t vec = conv.side2;

                for (size_t i = sizeof(size_t); i < msg.len; i+=sizeof(size_t)) {
                    std::memcpy(conv.side1, &msg.data[i], sizeof(size_t));
                    map[vec].insert(conv.side2);
                }
                delete[] msg.data;
                break;
            }
            case msg_types::CLIENT_DISCONNECT: {
                cl.remove(msg.peer_id);
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
    auto saver = select_saver(nullptr);
    saver->finalize(map);
    delete saver;
    delete parser;
}
