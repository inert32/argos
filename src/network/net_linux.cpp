// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#ifdef __linux__

#include <stdexcept>
#include <vector>
#include <thread>
#include <iostream>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>
#include "unistd.h"

#include "../settings.h"
#include "net_def.h"
#include "net_int.h"

bool netd_started = false;

#define throw_err(msg) throw std::runtime_error(std::string(msg) + ": " + strerror(errno))

bool init_network() { return true; }

void shutdown_network() {}

ipv4_t::ipv4_t(const sockaddr src) {
    sockaddr_in data = *(sockaddr_in*)&src;
    port = ntohs(data.sin_port);

    const auto addr_raw = inet_ntoa(data.sin_addr);
    if (addr_raw != nullptr) std::memcpy(ip, addr_raw, ipv4_ip_len);
}

socket_t::socket_t() {
    s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
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
        bool ok = get_msg(&check);
        if (!ok || check.type != msg_types::SERVER_CLIENT_ACCEPT)
            throw std::runtime_error("Connect error: server declined connection.");
    }
}

socket_t::socket_t(const socket_int_t old) {
    s = old;
}

bool socket_t::get_msg(net_msg* ret) {
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

bool socket_t::send_msg(const net_msg& msg) {
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

bool socket_t::send_msg(const msg_types msg) {
    header_t head;
    head.type = msg;

    auto bytes_send = send(s, &head, sizeof(header_t), 0);
    if (bytes_send < (signed)sizeof(header_t))
        std::cout << "send_msg: incomplete message send (" << bytes_send << " of " << sizeof(msg_types) << ")" << std::endl;

    return true;
}

bool socket_t::is_online() const {
    char c;
    auto l = recv(s, &c, sizeof(char), MSG_PEEK);

    return (l > 0 || (errno == EAGAIN || errno == EWOULDBLOCK)) ? true : false;
}

void socket_t::set_nonblock() {
    fcntl(s, F_SETFL, O_NONBLOCK);
}

socket_int_t socket_t::raw() {
    return s;
}

socket_t* socket_t::accept_conn(ipv4_t* who) {
    sockaddr addr; socklen_t len = sizeof(sockaddr);

    auto try_accept = accept(s, &addr, &len);
    if (try_accept > 0) {
        if (who != nullptr) *who = ipv4_t(addr);
        return new socket_t(try_accept);
    }
    else return nullptr;
}

void socket_t::kill() {
    close(s);
}

#endif /* __linux__ */