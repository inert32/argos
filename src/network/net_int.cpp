// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <iostream>
#include "../settings.h"
#include "net_int.h"

size_t clients_now = 0;

clients_list::clients_list() {
    list.reserve(clients_max);
    for (size_t i = 0; i < clients_max; i++)
        list.push_back(nullptr);
}

clients_list::~clients_list() {
    for (size_t i = 0; i < clients_max; i++) 
        if (list[i] != nullptr) socket_close(*list[i]);
}

bool clients_list::try_add(const socket_int_t s) {
    if (clients_count == clients_max) return false;
    for (size_t i = 0; i < clients_max; i++) 
        if (list[i] == nullptr) {
            socket_set_nonblock(s);
            list[i] = new socket_int_t;
            *list[i] = s;
            
            clients_count++;
            wait_for_clients = false;
            std::cout << "Accepted client" << std::endl;
            return true;
        }
    return false;
}

void clients_list::remove(const size_t id) {
    delete list[id];
    list[id] = nullptr;
    clients_count--;
    std::cout << "Client disconnect" << std::endl;
}

socket_int_t* clients_list::get(const size_t id) const {
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