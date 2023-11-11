// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <stdexcept>
#include <vector>
#include <thread>
#include <iostream>

#ifdef __linux__
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>
#include "unistd.h"
#endif /* __linux__ */

#include "settings.h"
#include "net_def.h"
#include "net_int.h"

bool netd_started = false;
std::array<socket_t*, clients_max> clients_list;

#define throw_err(msg) throw std::runtime_error(std::string(msg) + ": " + strerror(errno))

socket_t::socket_t() {
    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) throw_err("Socket error");

    sockaddr_in addr;
    addr.sin_family = AF_INET;

    if (master_mode) {
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_port=htons(port_server);
        fcntl(s, F_SETFL, O_NONBLOCK);
        if (bind(s, (sockaddr*)&addr, sizeof(addr)) < 0) throw_err("Bind error");
        if (listen(s, 1) == -1) throw_err("Listen fail");
    }
    else {
        addr.sin_addr.s_addr = inet_addr(master_addr.ip);
        addr.sin_port=htons(master_addr.port);
        if (connect(s, (sockaddr *)&addr, sizeof(addr)) < 0) throw_err("Connect error");

        char server_ans = 0;
        auto got = recv(s, &server_ans, 1, 0);
        std::cout << "Bytes: " << got << ", raw: " << (int)server_ans << std::endl;
        if ((msg_types)server_ans != msg_types::SERVER_CLIENT_ACCEPT)
            throw std::runtime_error("Connect error: server declined connection.");
    }
}

socket_t::socket_t(const socket_int_t old) {
    s = old;
    fcntl(s, F_SETFL, O_NONBLOCK);
}

socket_t::operator socket_int_t() {
    return s;
}

socket_t::~socket_t() {
    close(s);
}

void socket_t::reset(const socket_int_t old){
    s = old;
    fcntl(s, F_SETFL, O_NONBLOCK);
}

bool socket_t::get_msg(net_msg* ret) {
    char c;
    // Получаем длину сообщения
    auto l = recv(s, &c, sizeof(char), MSG_PEEK | MSG_TRUNC);
    if (l < (signed)sizeof(msg_types)) return false;
    size_t len_raw = (size_t)l; // Так как recv() вовращает знаковый тип

    // Выделяем место под буфер
    char* msg_raw = new char[len_raw];
    recv(s, msg_raw, len_raw, 0);

    // Копируем данные сообщения
    conv_t<msg_types> conv;
    std::memcpy(conv.side1, &msg_raw, sizeof(msg_types));
    ret->type = conv.side2;
    size_t len = len_raw - sizeof(msg_types);
    ret->len = len;

    std::cout << "msg data: type " << (int)conv.side2 << ", len: " << len_raw << std::endl;

    // Копируем сообщение в ret
    if (len > 0) {
        ret->data = new char[len];
        std::memcpy(ret->data, &msg_raw[sizeof(msg_types)], len);
    }
    return true;
}

bool socket_t::send_msg(const net_msg& msg) {
    const auto offset = sizeof(msg_types);
    const auto raw_len = offset + msg.len;

    char* msg_raw = new char[raw_len];
    conv_t<msg_types> conv;
    conv.side2 = msg.type;
    std::memcpy(msg_raw, &conv.side1, sizeof(msg_types));

    std::memcpy(&msg_raw[offset], msg.data, msg.len);
    return (send(s, &msg_raw, raw_len, 0) > 0);
}

bool socket_t::send_msg(const msg_types msg) {
    conv_t<msg_types> conv;
    conv.side2 = msg;
    return (send(s, &conv.side1, sizeof(msg_types), 0) > 0);
}

bool socket_t::send_msg(const char* msg, const size_t msg_len) {
    return (send(s, msg, msg_len, 0) > 0);
}

size_t clients_now = 0;

size_t find_slot() {
    for (size_t i = 0; i < clients_max; i++)
        if (clients_list[i] == nullptr) return i;
    return clients_max;
}

void netd_server(socket_t* sock_in, th_queue<net_msg>* queue, volatile bool* run) {
    netd_started = true;

    while (*run == true) {
        socket_int_t try_accept = accept(*sock_in, nullptr, nullptr);
        if (try_accept >= 0) {
            auto i = find_slot();
            if (i == clients_max) {
                std::cout << "Declined client" << std::endl;
                socket_t tmp(try_accept);
                tmp.send_msg(msg_types::SERVER_CLIENT_NOT_ACCEPT);
            }
            else {
                std::cout << "Accepted client" << std::endl;
                clients_list[i] = new socket_t(try_accept);
                clients_list[i]->send_msg(msg_types::SERVER_CLIENT_ACCEPT);
            }
        }
        for (size_t i = 0; i < clients_max; i++) {
            if (clients_list[i] == nullptr) continue;
            net_msg ret;
            if (!clients_list[i]->get_msg(&ret)) continue;
            std::cout << "Client request" << std::endl;
            ret.peer_id = i;
            queue->add(ret);
        }
    }
    netd_started = false;
}