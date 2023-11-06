// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <iostream>
#include <cstring>
#include "clients.h"

clients_list::clients_list(const size_t count) {
    clients_current = 0;
    clients_max = count;

    array = new ipv4_t*[count];
    for (size_t i = 0; i < count; i++) array[i] = nullptr;
}

clients_list::~clients_list() {
    for (size_t i = 0; i < clients_max; i++) delete array[i];
    delete[] array;
}

bool clients_list::add(const char* ip, const size_t port) {
    if (clients_current + 1 < clients_max) {
        size_t index = 0;
        if (in_array(ip, &index)) {
            std::cout << "clients_add: move port " << array[index]->port << "->" << port
            << " for ip " << array[index]->ip << std::endl;
            array[index]->port = port;
        }
        else {
            index = avaliable_slot();
            array[index] = new ipv4_t;
            std::memcpy(array[index]->ip, ip, ipv4_ip_len);
            array[index]->port = port;
            std::cout << "clients_add: add client " << array[index]->ip << ":" << array[index]->port << " (slot " << index << ")" << std::endl;
        }      
        return true;
    }
    else return false;
}

bool clients_list::add(const ipv4_t& ip) {
    return add(ip.ip, ip.port);
}

void clients_list::remove(const char* ip) {
    size_t index = 0;
    if (!in_array(ip, &index)) return;
    std::cout << "clients_add: remove client " << array[index]->ip << ":" << array[index]->port << std::endl;
    delete array[index];
    array[index] = nullptr;
}

void clients_list::remove(const size_t index) {
    if (index > clients_max) return;
    std::cout << "clients_add: add port " << array[index]->ip << ":" << array[index]->port << std::endl;
    delete array[index];
    array[index] = nullptr;
}

bool clients_list::in_array(const char* ip, size_t* index = nullptr) {
    size_t ret = find_ip(ip);

    size_t ret_index = 0;
    bool ret_value;
    if (ret < clients_max) {
        ret_index = ret;
        ret_value = true;
    }
    else {
        ret_index = (size_t)-1;
        ret_value = false;
    }

    if (index != nullptr) *index = ret_index;
    return ret_value;
}

ipv4_t clients_list::find_by_ip(const char* ip, size_t* index = nullptr) {
    size_t i = find_ip(ip);
    if (i < clients_max) {
        if (index != nullptr) *index = i;
        return *array[i];
    }

    ipv4_t dummy;
    return dummy;
}

size_t clients_list::avaliable_slot() {
    for (size_t i = 0; i < clients_max; i++)
        if (array[i] == nullptr) return i;
    return (size_t)-1;
}

size_t clients_list::find_ip(const char* ip) {
    for (size_t i = 0; i < clients_max; i++)
        if (array[i] != nullptr && std::memcmp(array[i]->ip, ip, ipv4_ip_len) == 0) 
            return i;
    return (size_t)-1;
}