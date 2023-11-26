// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <thread>
#include <iostream>

#ifdef __linux__
#include <sys/poll.h>
#elif _WIN32
#include <WinSock2.h>
#endif

#include "../settings.h"
#include "net_int.h"

clients_list::clients_list() {
    list.reserve(clients_max);
}

clients_list::~clients_list() {
    for (auto &i : list) i->kill();
}

bool clients_list::try_add(socket_t* s) {
    if (clients_count == clients_max) return false;

    list.push_back(s);
    list.back()->set_nonblock();
    clients_count++;
    wait_for_clients = false;
    return true;
}

void clients_list::remove(const size_t id) {
    list[id]->kill();
    list.erase(list.begin() + id);
    clients_count--;
}

socket_t* clients_list::get(const size_t id) const {
    return list[id];
}

size_t clients_list::count() const {
    return clients_count;
}

std::vector<size_t> clients_list::poll_sockets(socket_t* conn_socket, bool* new_client) const {
    // Используем poll для опроса сокетов
    const auto size = clients_count + 1;
    pollfd* poll_list = new pollfd[size];

    // Отдельно обрабатываем слушающий сокет
    size_t ind = 0;
    poll_list[ind].fd = conn_socket->raw();
    poll_list[ind++].events = POLLIN;
    for (auto &i : list) {
        poll_list[ind].fd = i->raw();
        poll_list[ind++].events = POLLIN;
    }
    // Ждем секунду
    auto poll_ret = poll(poll_list, size, 10000);

    std::vector<size_t> ret;
    if (poll_ret == -1) std::cerr << "netd: poll error: " << errno << std::endl;
    if (poll_ret > 0) {
        if (poll_list[0].revents & POLLIN) *new_client = true;
        for (size_t i = 0; i < clients_count; i++)
            if (poll_list[i + 1].revents & POLLIN) ret.push_back(i);
    }

    delete[] poll_list;
    return ret;
}

bool clients_list::run_server() const {
    return wait_for_clients || clients_count > 0;
}

int calc_checksum(const net_msg& msg) {
    unsigned char sum = 0;
    for (size_t i = 0; i < msg.len; i++) sum ^= msg.data[i];
    return (int)sum;
}

void netd_server(socket_t* sock_in, clients_list* clients, th_queue<net_msg>* queue, volatile bool* run) {
    netd_started = true;

    while (*run == true) {
        bool incoming = false;
        auto cl = clients->poll_sockets(sock_in, &incoming);

        // Подключение нового клиента
        if (incoming) {
            ipv4_t client_data;
            auto try_accept = sock_in->accept_conn(&client_data);
            if (try_accept != nullptr) {
                if (clients->try_add(try_accept)) {
                    std::cout << "Accepted client " << client_data << std::endl;
                    try_accept->send_msg(msg_types::SERVER_CLIENT_ACCEPT);
                }
                else try_accept->send_msg(msg_types::SERVER_CLIENT_NOT_ACCEPT);
            }
        }
        
        if (clients->count() < clients_min) continue;

        // Добавляем запросы подключенных клиентов в очередь
        for (auto &i : cl) {
            auto c = clients->get(i);
            if (!(c->is_online())) {
                clients->remove(i);
                continue;
            }

            net_msg ret;
            if (!c->get_msg(&ret)) continue;
            ret.peer_id = i;
            queue->add(ret);
        }
    }
    netd_started = false;
}