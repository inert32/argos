// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <iostream>
#include "../settings.h"
#include "net_int.h"

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
            return true;
        }
    return false;
}

void clients_list::remove(const size_t id) {
    list.erase(list.begin() + id);
    clients_count--;
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