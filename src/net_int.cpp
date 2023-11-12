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
size_t clients_now = 0;
bool wait_for_clients = true;

constexpr auto header_len = sizeof(msg_types) + sizeof(size_t);

#define throw_err(msg) throw std::runtime_error(std::string(msg) + ": " + strerror(errno))

bool run_server() {
    return wait_for_clients || clients_now > 0;
}

char* mk_header(const size_t len, const msg_types type) {
    char* ret = new char[header_len];

    conv_t<size_t> conv_len;
    conv_len.side2 = len;

    conv_t<msg_types> conv_type;
    conv_type.side2 = type;

    std::memcpy(ret, conv_len.side1, sizeof(size_t));
    std::memcpy(&ret[sizeof(size_t)], conv_type.side1, sizeof(msg_types));

    return ret;
}

socket_int_t socket_setup() {
    socket_int_t s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) throw_err("Socket error");

    sockaddr_in addr;
    addr.sin_family = AF_INET;

    if (master_mode) {
        fcntl(s, F_SETFL, O_NONBLOCK);
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_port=htons(port_server);
        if (bind(s, (sockaddr*)&addr, sizeof(addr)) < 0) throw_err("Bind error");
        if (listen(s, 1) == -1) throw_err("Listen fail");
    }
    else {
        addr.sin_addr.s_addr = inet_addr(master_addr.ip);
        addr.sin_port=htons(master_addr.port);
        if (connect(s, (sockaddr *)&addr, sizeof(addr)) < 0) throw_err("Connect error");

        net_msg check;
        bool ok = socket_get_msg(s, &check);
        if (!ok || check.type != msg_types::SERVER_CLIENT_ACCEPT)
            throw std::runtime_error("Connect error: server declined connection.");
    }
    return s;
}

void socket_close(socket_int_t s) {
    close(s);
}

bool socket_online(socket_int_t s) {
    char c;
    auto l = recv(s, &c, sizeof(char), MSG_PEEK);

    return (l == 0) ? true : false;
}

bool socket_get_msg(socket_int_t s, net_msg* ret) {
    char header_raw[header_len] = "\0\0\0\0\0\0\0\0";
    auto l = recv(s, header_raw, header_len, 0);
    if (l < (signed)header_len) return false;

    conv_t<size_t> raw_size; conv_t<msg_types> raw_type;
    for (size_t i = 0; i < sizeof(size_t); i++) raw_size.side1[i] = header_raw[i];
    for (size_t i = 0; i < sizeof(msg_types); i++) raw_type.side1[i] = header_raw[i + sizeof(size_t)];

    ret->type = raw_type.side2;
    ret->len = raw_size.side2 - header_len;

    if (ret->len == 0) return true;

    // Копируем сообщение в ret
    ret->data = new char[ret->len];
    l = recv(s, ret->data, ret->len, 0);
    if (l < (signed)ret->len) std::cout << "get_msg: incomplete message got (" << l << " of " << ret->len << ")" << std::endl;
    return true;
}

bool socket_send_msg(socket_int_t s, const net_msg& msg) {
    const auto raw_len = msg.len + header_len;
    char* msg_raw = new char[raw_len];

    auto header = mk_header(raw_len, msg.type);
    std::memcpy(msg_raw, header, header_len);
    delete[] header;

    if (msg.len > 0) std::memcpy(&msg_raw[header_len], msg.data, msg.len);

    auto send_bytes = send(s, msg_raw, raw_len, 0);
    if (send_bytes < (signed)raw_len) std::cout << "send_msg: incomplete message send (" << send_bytes << " of " << raw_len << ")" << std::endl;

    delete[] msg_raw;
    return (send_bytes > 0);
}

bool socket_send_msg(socket_int_t s, const msg_types msg) {
    auto header = mk_header(header_len, msg);

    auto send_bytes = send(s, header, header_len, 0);
    if (send_bytes < (signed)sizeof(msg_types)) std::cout << "send_msg: incomplete message send (" << send_bytes << " of " << sizeof(msg_types) << ")" << std::endl;

    delete[] header;
    return (send_bytes > 0);
}

size_t find_slot(socket_int_t** clients) {
    for (size_t i = 0; i < clients_max; i++)
        if (clients[i] == nullptr) return i;
    return clients_max;
}

void netd_server(socket_int_t sock_in, socket_int_t** clients, th_queue<net_msg>* queue, volatile bool* run) {
    netd_started = true;

    while (*run == true) {
        std::this_thread::sleep_for(std::chrono::microseconds(10));
        socket_int_t try_accept = accept(sock_in, nullptr, nullptr);
        if (try_accept > 0) {
            auto i = find_slot(clients);
            if (i == clients_max) {
                std::cout << "Declined client" << std::endl;
                socket_send_msg(try_accept, msg_types::SERVER_CLIENT_NOT_ACCEPT);
            }
            else {
                std::cout << "Accepted client" << std::endl;
                clients[i] = new socket_int_t;
                *clients[i] = try_accept;
                fcntl(*clients[i], F_SETFL, O_NONBLOCK);
                clients_now++;
                wait_for_clients = false;
                std::cout << "Clients count: " << clients_now << std::endl;
                socket_send_msg(try_accept, msg_types::SERVER_CLIENT_ACCEPT);
            }
        }
        for (size_t i = 0; i < clients_max; i++) {
            if (clients[i] == nullptr) continue;
            if (!socket_online(*clients[i]) && !(errno == EAGAIN || errno == EWOULDBLOCK)) {
                std::cout << "Client disconnected" << std::endl;
                socket_close(*clients[i]);
                clients[i] = nullptr;
                clients_now--;
                std::cout << "Clients count: " << clients_now << std::endl;
                continue;
            }

            net_msg ret;
            if (!socket_get_msg(*clients[i], &ret)) continue;
            ret.peer_id = i;
            queue->add(ret);
        }
    }
    netd_started = false;
}