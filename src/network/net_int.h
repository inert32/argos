#ifndef __NET_INT_H__
#define __NET_INT_H__

/*
    Внутренние функции работы с сетью
*/

#include <iostream>
#include "net_def.h"
#include "../th_queue.h"

struct header_t {
    size_t raw_len = sizeof(header_t);
    msg_types type = msg_types::BOTH_UNKNOWN;
};

class clients_list {
public:
    clients_list();
    ~clients_list();

    bool try_add(socket_t* s);
    void remove(const size_t id);
    socket_t* get(const size_t id) const;
    size_t count() const;
    std::vector<size_t> poll_sockets() const;

    bool run_server() const;
private:
    bool wait_for_clients = true;
    size_t clients_count = 0;
    std::vector<socket_t*> list;
};

class socket_t {
public:
    socket_t();
    socket_t(const socket_int_t old);

    bool get_msg(net_msg* ret);

    bool send_msg(const net_msg& msg);
    bool send_msg(const msg_types msg);

    bool is_online() const;
    void set_nonblock();

    socket_t* accept_conn(ipv4_t* who);

    socket_int_t raw();

    void kill();
private:
    socket_int_t s;
};

int calc_checksum(const net_msg& msg);

// Флаг работы сетевого потока (только для сервера)
extern bool netd_started;

constexpr size_t net_chunk_size = 1024;

// Флаг работы сервера
// Ждет подключения хотя бы одного клиента, затем отключения всех клиентов
bool run_server();

bool init_network();
void shutdown_network();

// Сетевой поток
void netd_server(socket_t* sock_in, clients_list* clients, th_queue<net_msg>* queue, volatile bool* run);

#endif /* __NET_INT_H__ */