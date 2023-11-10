// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <iostream>
#include <stdexcept>
#include <thread>

#ifdef __linux__
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>
#include "unistd.h"
#endif /* __linux__ */

#include "settings.h"
#include "th_queue.h"
#include "net.h"

#define throw_err(msg) throw std::runtime_error(std::string(msg) + ": " + strerror(errno))

unsigned int clients_max = 10;
unsigned int port_server = 3700;

unsigned int clients_now = 0;
bool netd_started = false;

int setup_socket() {
    int ret = socket(AF_INET, SOCK_STREAM, 0);
    if (ret < 0) throw_err("Socket error");
    if (master_mode) fcntl(ret, F_SETFL, O_NONBLOCK);

    sockaddr_in addr;
    addr.sin_family = AF_INET;

    if (master_mode) {
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_port=htons(port_server);
        if (bind(ret, (sockaddr*)&addr, sizeof(addr)) < 0) throw_err("Bind error");
    }
    else {
        std::cout << "Connect to " << master_addr << std::endl;
        addr.sin_addr.s_addr = inet_addr(master_addr.ip);
        addr.sin_port=htons(master_addr.port);
        if (connect(ret, (sockaddr *)&addr, sizeof(addr)) < 0) throw_err("Connect error");
    }
    return ret;
}

void close_socket(int socket) {
    close(socket);
}

bool get_msg(int socket, net_msg* ret) {
    char msg_raw[8192];
    int got_bytes = recv(socket, &msg_raw, sizeof(msg_raw), 0);
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

unsigned int find_slot(const int* sock_list) {
    for (unsigned int i = 0; i < clients_max; i++)
        if (sock_list[i] == -2) return i;
    return (unsigned int)-1;
}

void netd_server(int* sock_in, int* sock_list, th_queue<net_msg>* queue, volatile bool* run) {
    const char accepted = (char)msg_types::SERVER_CLIENT_ACCEPT;
    const char not_accepted = (char)msg_types::SERVER_CLIENT_NOT_ACCEPT;
    netd_started = true;

    while (*run == true) {
        int try_accept = accept(*sock_in, nullptr, nullptr);
        if (try_accept >= 0) {
            if (clients_now < clients_max) {
                auto ind = find_slot(sock_list);
                sock_list[ind] = try_accept;
                clients_now++;
                send(try_accept, &accepted, 1, 0);
            }
            else send(try_accept, &not_accepted, 1, 0);
        }
        for (unsigned int i = 0; i < clients_max; i++) {
            if (sock_list[i] < 0) continue;

            net_msg ret;
            if (!get_msg(sock_list[i], &ret)) continue;
            ret.peer_id = i;
            queue->add(ret);
        }
    }
    netd_started = false;
}

void master_start(int* socket) {
    std::cout << "Master mode" << std::endl;
    std::cout << "Listen port " << port_server << std::endl;

    int clients_list[10];
    for (auto &c : clients_list) c = -2;
    th_queue<net_msg> queue;
    volatile bool threads_run = true;

    if (listen(*socket, 1) == -1) std::cerr << "Listen fail: " << strerror(errno) << "(" << errno << ")" << std::endl;
    std::thread netd(netd_server, socket, clients_list, &queue, &threads_run);

    while (!netd_started) continue;

    std::cout << "Ready." << std::endl;
    while (true) {
        auto x = queue.take();
        if (!x) continue;
        auto msg = *x;
        auto send_to = clients_list[msg.peer_id];
        std::cout << "Type: " << (int)msg.type << std::endl;
        std::cout << "Len: " << msg.len << std::endl;
        switch (msg.type) {
            case msg_types::CLIENT_GET_VECTORS: {
                auto a = (char)msg_types::SERVER_DATA;
                send(send_to, &a, 1, 0);
                break;
            }
            case msg_types::CLIENT_GET_TRIANGLE: {
                auto a = (char)msg_types::SERVER_DONE;
                send(send_to, &a, 1, 0);
                break;
            }
            default: {
                auto a = (char)msg_types::BOTH_UNKNOWN;
                send(send_to, &a, 1, 0);
                break;
            }
        }
    }
    threads_run = false;
    while (netd_started) continue;
    netd.join();
}