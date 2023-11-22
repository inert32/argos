// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#ifdef _WIN32

#include <stdexcept>
#include <vector>
#include <thread>
#include <iostream>

#include <WinSock2.h>

#include "../settings.h"
#include "net_def.h"
#include "net_int.h"

bool netd_started = false;

#define throw_err(msg) \
    throw std::runtime_error(std::string(msg) + ": " + \
    strerror(WSAGetLastError()) + " (" + \
    std::to_string(WSAGetLastError()) + ")")

bool init_network() {
    WSADATA data;
    auto start = WSAStartup(MAKEWORD(2, 2), &data);
    if (start != 0) {
        std::cerr << "WSA: WSAStartup: " << start << std::endl;
        return false;
    }
    return true;
}

void shutdown_network() {
    WSACleanup();
}

socket_int_t socket_setup() {
    socket_int_t s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) throw_err("Socket error");

    sockaddr_in addr;
    addr.sin_family = AF_INET;

    if (master_mode) {
        u_long mode = 1;
        ioctlsocket(s, FIONBIO, &mode);
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
    closesocket(s);
}

bool socket_online(socket_int_t s) {
    char c;
    auto l = recv(s, &c, sizeof(char), MSG_PEEK);
    return (l == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK) ? false : true;
}

bool socket_get_msg(socket_int_t s, net_msg* ret) {
    header_t head;
    auto l = recv(s, (char*)&head, sizeof(header_t), 0);
    if (l == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK) {
        std::cerr << "get_msg: WSA return " << WSAGetLastError() << std::endl;
    }
    
    if (l < (signed)sizeof(header_t)) return false;

    ret->type = head.type;
    ret->len = head.raw_len - sizeof(header_t);

    if (ret->len == 0) return true;
    const auto flag = (master_mode) ? 0 : MSG_WAITALL;
    // Копируем сообщение в ret
    ret->data = new char[ret->len];
    size_t bytes_got = 0;
    while (bytes_got < ret->len) {
        // Принимаем блоками по net_chunk_size байт
        // Вычисляем размер следующего блока
        const size_t bytes_remain = ret->len - bytes_got;
        const size_t block_len = (bytes_remain > net_chunk_size) ? net_chunk_size : bytes_remain;

        // Принимаем блок
        auto got = recv(s, &ret->data[bytes_got], block_len, flag); // MSG_WAITALL заблокирует поток пока мы получаем данные
        if (got == SOCKET_ERROR) {
            auto err = WSAGetLastError();
            if (err == WSAEWOULDBLOCK) bytes_got++; // строка 101 уменьшит bytes_got, компенсируем
            else {
                std::cout << "get_msg: " << strerror(err) << " (" << err << ")" << std::endl;
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
        if (sent == SOCKET_ERROR) break;
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

    auto bytes_send = send(s, (char*)&head, sizeof(header_t), 0);
    if (bytes_send < (signed)sizeof(header_t))
        std::cout << "send_msg: incomplete message send (" << bytes_send << " of " << sizeof(msg_types) << ")" << std::endl;

    return true;
}

void socket_set_nonblock(const socket_int_t s) {
    u_long mode = 1;
    ioctlsocket(s, FIONBIO, &mode);
}

void netd_server(socket_int_t sock_in, clients_list* clients, th_queue<net_msg>* queue, volatile bool* run) {
    netd_started = true;

    while (*run == true) {
        std::this_thread::sleep_for(std::chrono::microseconds(10));
        socket_int_t try_accept = accept(sock_in, nullptr, nullptr);
        if (try_accept != INVALID_SOCKET) {
            if (clients->try_add(try_accept)) socket_send_msg(try_accept, msg_types::SERVER_CLIENT_ACCEPT);
            else socket_send_msg(try_accept, msg_types::SERVER_CLIENT_NOT_ACCEPT);
        }
        if (clients->count() < clients_min) continue;

        for (size_t i = 0; i < clients_max; i++) {
            auto c = clients->get(i);
            if (c == nullptr) continue;
            if (!socket_online(*c)) {
                clients->remove(i);
                continue;
            }

            net_msg ret;
            if (!socket_get_msg(*c, &ret)) continue;
            ret.peer_id = i;
            queue->add(ret);
        }
    }
    netd_started = false;
}

#endif /* _WIN32 */
