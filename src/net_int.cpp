// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <stdexcept>
#include <thread>

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
unsigned int clients_now = 0;

#define throw_err(msg) throw std::runtime_error(std::string(msg) + ": " + strerror(errno))

socket_t::socket_t() {
    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) throw_err("Socket error");
    if (master_mode) fcntl(s, F_SETFL, O_NONBLOCK);

    sockaddr_in addr;
    addr.sin_family = AF_INET;

    if (master_mode) {
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_port=htons(port_server);
        if (bind(s, (sockaddr*)&addr, sizeof(addr)) < 0) throw_err("Bind error");
        if (listen(s, 1) == -1) throw_err("Listen fail");
    }
    else {
        addr.sin_addr.s_addr = inet_addr(master_addr.ip);
        addr.sin_port=htons(master_addr.port);
        if (connect(s, (sockaddr *)&addr, sizeof(addr)) < 0) throw_err("Connect error");
    }
}

socket_t::socket_t(socket_int_t old) {
    s = old;
}

socket_t::operator socket_int_t() {
    return s;
}

socket_t::~socket_t() {
    close(s);
}

bool socket_t::get_msg(net_msg* ret) {
    char msg_raw[8192];
    int got_bytes = recv(s, &msg_raw, sizeof(msg_raw), 0);
    if (got_bytes < 1) return false;

    ret->type = (msg_types)msg_raw[0];
    got_bytes--;
    ret->len = got_bytes;
    if (got_bytes > 1) {
        ret->data = new char[got_bytes];
        std::memcpy(ret->data, &msg_raw[1], got_bytes);
    }
    return true;
}

bool socket_t::send_msg(const net_msg& msg) {
    char* msg_raw = new char[1 + msg.len];
    msg_raw[0] = (char)msg.type;
    std::memcpy(&msg_raw[1], msg.data, msg.len);

    return (send(s, &msg_raw, 1 + msg.len, 0) > 0);
}

bool socket_t::send_msg(const char* msg, const size_t& msg_len) {
    return (send(s, msg, msg_len, 0) > 0);
}

void netd_server(socket_t* sock_in, std::vector<socket_t>* sock_list, th_queue<net_msg>* queue, volatile bool* run) {
    const char accepted = (char)msg_types::SERVER_CLIENT_ACCEPT;
    const char not_accepted = (char)msg_types::SERVER_CLIENT_NOT_ACCEPT;
    netd_started = true;

    while (*run == true) {
        socket_int_t try_accept = accept(*sock_in, nullptr, nullptr);
        if (try_accept >= 0) {
            if (clients_now < clients_max) {
                sock_list->emplace_back(socket_t(try_accept));
                send(try_accept, &accepted, 1, 0);
            }
            else send(try_accept, &not_accepted, 1, 0);
        }
        for (auto &i : *sock_list) {
            net_msg ret;
            if (!i.get_msg(&ret)) continue;
            ret.peer_id = i;
            queue->add(ret);
        }
    }
    netd_started = false;
}