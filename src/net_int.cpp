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

struct header_t {
    size_t raw_len = sizeof(header_t);
    msg_types type = msg_types::BOTH_UNKNOWN;
};

#define throw_err(msg) throw std::runtime_error(std::string(msg) + ": " + strerror(errno))

bool run_server() {
    return wait_for_clients || clients_now > 0;
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
    header_t head;
    auto l = recv(s, &head, sizeof(header_t), 0);
    if (l < (signed)sizeof(header_t)) return false;

    ret->type = head.type;
    ret->len = head.raw_len - sizeof(header_t);

    if (ret->len == 0) return true;

    // Копируем сообщение в ret
    ret->data = new char[ret->len];
    size_t bytes_got = 0;
    while (bytes_got < ret->len) {
        // Принимаем блоками по net_chunk_size байт
        // Вычисляем размер следующего блока
        const size_t bytes_remain = ret->len - bytes_got;
        const size_t block_len = (bytes_remain > net_chunk_size) ? net_chunk_size : bytes_remain;

        // Принимаем блок
        auto got = recv(s, &ret->data[bytes_got], block_len, MSG_WAITALL); // MSG_WAITALL заблокирует поток пока мы получаем данные
        if (got == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) bytes_got++; // строка 101 уменьшит bytes_got, компенсируем
            else {
                std::cout << "get_msg: " << strerror(errno) << " (" << errno << ")" << std::endl;
                break;
            }
        }
        bytes_got+=got;
    }
    if (bytes_got < ret->len)
        std::cout << "get_msg: incomplete message got: " << bytes_got << " of " << ret->len << std::endl;

    return true;
}

bool socket_send_msg(socket_int_t s, const net_msg& msg) {
    header_t head;
    head.type = msg.type;
    head.raw_len += msg.len;
    const size_t raw_len = sizeof(header_t) + msg.len;

    char* msg_raw = new char[raw_len];
    std::memcpy(msg_raw, &head, sizeof(header_t));

    std::memcpy(&msg_raw[sizeof(header_t)], msg.data, msg.len);

    size_t bytes_send = 0;
    while (bytes_send < raw_len) {
        // Отправляем блоками по net_chunk_size байт
        // Вычисляем размер следующего блока
        const size_t bytes_remain = raw_len - bytes_send;
        const size_t block_len = (bytes_remain > net_chunk_size) ? net_chunk_size : bytes_remain;

        // Отправляем блок
        auto sent = send(s, &msg_raw[bytes_send], block_len, 0);
        if (sent == -1) break;
        bytes_send+=sent;
    }
    if (bytes_send < raw_len)
        std::cout << "send_msg: incomplete message sent: " << bytes_send << " of " << raw_len << std::endl;

    delete[] msg_raw;
    return true;
}

bool socket_send_msg(socket_int_t s, const msg_types msg) {
    header_t head;
    head.type = msg;

    auto send_bytes = send(s, &head, sizeof(header_t), 0);
    if (send_bytes < (signed)sizeof(header_t))
        std::cout << "send_msg: incomplete message send (" << send_bytes << " of " << sizeof(msg_types) << ")" << std::endl;

    return true;
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