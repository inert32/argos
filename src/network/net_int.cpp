// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <thread>
#include "../settings.h"
#include "net_int.h"

size_t clients_now = 0;

clients_list::clients_list() {
    list.reserve(clients_max);
    for (size_t i = 0; i < clients_max; i++) list.push_back(nullptr);
}

clients_list::~clients_list() {
    for (size_t i = 0; i < clients_max; i++)
        if (list[i] != nullptr) list[i]->kill();
}

bool clients_list::try_add(socket_t* s) {
    if (clients_count == clients_max) return false;
    for (size_t i = 0; i < clients_max; i++) 
        if (list[i] == nullptr) {
            list[i] = std::move(s);
            list[i]->set_nonblock();
            
            clients_count++;
            wait_for_clients = false;
            return true;
        }
    return false;
}

void clients_list::remove(const size_t id) {
    list[id]->kill();
    list[id] = nullptr;
    clients_count--;
}

socket_t* clients_list::get(const size_t id) const {
    if (id >= list.size()) return nullptr;
    return list[id];
}

size_t clients_list::count() const {
    return clients_count;
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
        std::this_thread::sleep_for(std::chrono::microseconds(50));

        ipv4_t client_data;
        auto try_accept = sock_in->accept_conn(&client_data);
        if (try_accept != nullptr) {
            if (clients->try_add(try_accept)) {
                std::cout << "Accepted client " << client_data << std::endl;
                try_accept->send_msg(msg_types::SERVER_CLIENT_ACCEPT);
            }
            else try_accept->send_msg(msg_types::SERVER_CLIENT_NOT_ACCEPT);
        }
        if (clients->count() < clients_min) continue;

        for (size_t i = 0; i < clients_max; i++) {
            auto c = clients->get(i);
            if (c == nullptr) continue;
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