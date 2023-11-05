// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <iostream>
#include <string.h>
#include "net.h"
#include "io.h"
#include "base.h"
#include "convert.h"

ipv4_t master_addr;

void ipv4_t::from_string(const std::string& str) {
    auto pos = str.find(':');
    std::string real_ip;
    if (pos != str.npos) {
        port = std::stoi(str.substr(pos + 1));
        real_ip = str.substr(0, pos);
    }
    else real_ip = str;
    const auto len = real_ip.length();

    for (size_t i = 0; i < len; i++) ip[i] = real_ip[i];
}

std::string ipv4_t::to_string() {
    std::string ret;
    for (int i = 0; i < ipv4_ip_len; i++) ret.push_back(ip[i]);
    return ret + ":" + std::to_string(port);
}

bool ipv4_t::is_set() {
    bool ret = false;
    for (int i = 0; i < ipv4_ip_len; i++)
        if (ip[i] != '\0') { ret = true; break; }
    return ret;
}

void master_start(socket_int* socket) {
    std::cout << "MASTER MODE" << std::endl;
    try {
        bool run = true;
        while (run) {
            net_envelope msg;
            socket->get_msg(&msg);
            std::cout << "Got message." << std::endl;
            std::cout << "From: " << msg.from << std::endl;
            std::cout << "Type: " << (int)msg.type << std::endl;
            std::cout << "Len: " << msg.msg_raw.len << std::endl;
        }
    }
    catch (const std::runtime_error& e) {
		std::cerr << "err: " << e.what() << std::endl;
		return;
    }
}

reader_network::reader_network(socket_int* socket) {
    if (!master_addr.is_set()) throw std::runtime_error("reader_network: No master address provided");
    conn = socket;

    net_envelope start;
    start.to = master_addr;
    start.type = msg_types::CLIENT_CONNECT;
    // Передаем серверу порт для подключения к нам
    start.msg_raw.data = new char[4];
    conv_t<size_t> conv;
    conv.side2 = 3700;
    for (int i = 0; i < 4; i++) start.msg_raw.data[i] = conv.side1[i];

    conn->send_msg(&start);

    net_envelope ans;
    conn->get_msg(&ans);
    if (ans.type == msg_types::SERVER_CLIENT_NOT_ACCEPT) 
        throw std::runtime_error("reader_network: Master server refused to connect!");
    else if (ans.type != msg_types::SERVER_CLIENT_ACCEPT) 
        throw std::runtime_error("reader_network: Unknown command from master server");
}

reader_network::~reader_network() {
    net_envelope finish;
    finish.to = master_addr;
    finish.type = msg_types::CLIENT_DISCONNECT;
    conn->send_msg(&finish);
    conn = nullptr;
}

bool reader_network::get_next_triangle(triangle* ret) {
    net_envelope msg, ans;
    msg.to = master_addr;
    msg.type = msg_types::CLIENT_GET_TRIANGLES;
    conn->send_msg(&msg);

    conn->get_msg(&ans);
    if (ans.type == msg_types::SERVER_DATA) {
        *ret = conv_cap_to_tr(&ans.msg_raw);
        return true;
    }
    else return false;
}

bool reader_network::get_next_vector(vec3* ret) {
    net_envelope msg, ans;
    msg.to = master_addr;
    msg.type = msg_types::CLIENT_GET_TRIANGLES;
    conn->send_msg(&msg);

    conn->get_msg(&ans);
    if (ans.type == msg_types::SERVER_DATA) {
        *ret = conv_cap_to_vec(&ans.msg_raw);
        return true;
    }
    else return false;
}

bool reader_network::have_triangles() const {
    return true;
}
bool reader_network::have_vectors() const {
    return true;
}

saver_network::saver_network(socket_int* socket) {
    conn = socket;
}
saver_network::~saver_network() {
    conn = nullptr;
}

void saver_network::save_tmp([[maybe_unused]] volatile char** mat, [[maybe_unused]] const unsigned int count) {

}
void saver_network::save_final() {

}