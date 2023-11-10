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

unsigned int clients_max = 10;
unsigned int port_server = 3700;

unsigned int clients_now = 0;
bool netd_started = false;

int setup_socket() {
    int ret = socket(AF_INET, SOCK_STREAM, 0);
    if (master_mode) fcntl(ret, F_SETFL, O_NONBLOCK);
    if (ret < 0) {
        std::cerr << "Socket error: " << strerror(errno) << "(" << errno << ")" << std::endl;
        return -1;
    }

    sockaddr_in addr;
    addr.sin_family = AF_INET;

    if (master_mode) {
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_port=htons(port_server);
        if (bind(ret, (sockaddr*)&addr, sizeof(addr)) < 0) {
            std::cerr << "Bind error: " << strerror(errno) << "(" << errno << ")" << std::endl;
            return -1;
        }
    }
    else {
        std::cout << "Connect to " << master_addr << std::endl;
        addr.sin_addr.s_addr = inet_addr(master_addr.ip);
        addr.sin_port=htons(master_addr.port);
        if (connect(ret, (sockaddr *)&addr, sizeof(addr)) < 0) {
            std::cerr << "Connect error: " << strerror(errno) << "(" << errno << ")" << std::endl;
            return -1;
        }
    }
    return ret;
}

unsigned int find_slot(const int* sock_list) {
    for (unsigned int i = 0; i < clients_max; i++)
        if (sock_list[i] == -2) return i;
    return (unsigned int)-1;
}

void netd_server(int* sock_in, int* sock_list, th_queue<net_msg>* queue, volatile bool* run) {
    const char accepted = (char)msg_types::SERVER_CLIENT_ACCEPT;
    const char not_accepted = (char)msg_types::SERVER_CLIENT_NOT_ACCEPT;
    char msg_raw[8192];
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

            size_t got_bytes = recv(sock_list[i], &msg_raw, sizeof(msg_raw), 0);
            if (got_bytes < 1) continue;

            net_msg ret;
            ret.peer_id = i;
            ret.type = (msg_types)msg_raw[0];
            ret.len = got_bytes - 1;
            if (ret.len > 0) {
                ret.data = new char[ret.len];
                std::memcpy(ret.data, &msg_raw[1], ret.len);
            }
            queue->add(ret);
        }
    }
    netd_started = false;
}

void master_start() {
    std::cout << "Master mode" << std::endl;
    int sock = setup_socket();
    if (sock == -1) return;
    std::cout << "Listen port " << port_server << std::endl;

    int clients_list[10];
    for (auto &c : clients_list) c = -2;
    th_queue<net_msg> queue;
    volatile bool threads_run = true;

    if (listen(sock, 1) == -1) std::cerr << "Listen fail: " << strerror(errno) << "(" << errno << ")" << std::endl;
    std::thread netd(netd_server, &sock, clients_list, &queue, &threads_run);

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

void client_start() {
    std::cout << "Client mode" << std::endl;
    int sock = setup_socket();
    if (sock == -1) return;
    std::cout << "Connecting to " << master_addr << std::endl;

    union { char side1[sizeof(size_t)]; size_t side2; } conv;
    conv.side2 = (int)msg_types::CLIENT_CONNECT;

    std::cout << "Trying to send..." << std::endl;
    sockaddr_in dest;
    dest.sin_family = AF_INET;
    dest.sin_port=htons(port_server);
    dest.sin_addr.s_addr = inet_addr(master_addr.ip);

    auto status = sendto(sock, conv.side1, sizeof(size_t), 0, (sockaddr*)&dest, sizeof(dest));
    if (status <= 0) std::cerr << "Send error: " << strerror(errno) << "(" << errno << ")" << std::endl;
    std::cout << "Waiting for reply..." << std::endl;

    net_msg msg;
    char msg_raw[1024];
    int got_bytes = recv(sock, &msg_raw, sizeof(msg_raw), 0);
    std::cout << "Got bytes: " << got_bytes << std::endl;
    if (got_bytes < 1) {
        std::cerr << "Empty message from server" << std::endl;
    }

    msg.type = (msg_types)msg_raw[0];
    std::cout << "Type: " << (int)msg.type << std::endl;
    msg.len = got_bytes - 1;
    std::cout << "Len: " << msg.len << std::endl;
    if (msg.len > 1) {
        msg.data = new char[msg.len - 1];
        std::memcpy(msg.data, &msg_raw[1], msg.len);
    }

    // another message

    auto msg_data2 = (char)msg_types::CLIENT_GET_VECTORS;
    status = sendto(sock, &msg_data2, sizeof(char), 0, (sockaddr*)&dest, sizeof(dest));
    if (status <= 0) std::cerr << "Send error: " << strerror(errno) << "(" << errno << ")" << std::endl;
    std::cout << "Waiting for reply..." << std::endl;

    got_bytes = recv(sock, &msg_raw, sizeof(msg_raw), 0);
    std::cout << "Got bytes: " << got_bytes << std::endl;
    if (got_bytes < 1) {
        std::cerr << "Empty message from server" << std::endl;
    }

    msg.type = (msg_types)msg_raw[0];
    std::cout << "Type: " << (int)msg.type << std::endl;
    msg.len = got_bytes - 1;
    std::cout << "Len: " << msg.len << std::endl;
    if (msg.len > 1) {
        msg.data = new char[msg.len - 1];
        std::memcpy(msg.data, &msg_raw[1], msg.len);
    }

    close(sock);
}