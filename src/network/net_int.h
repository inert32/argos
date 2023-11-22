#ifndef __NET_INT_H__
#define __NET_INT_H__

/*
    Внутренние функции работы с сетью
*/

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

    bool try_add(const socket_int_t s);
    void remove(const size_t id);
    socket_int_t* get(const size_t id) const;
    size_t count() const;

    bool run_server() const;
private:
    bool wait_for_clients = true;
    size_t clients_count = 0;
    std::vector<socket_int_t*> list;
};

// Функции работы с сокетами
// TODO: Переработать в класс
// Создать сокет
socket_int_t socket_setup();
// Закрыть сокет
void socket_close(socket_int_t s);

// Получить сообщение
bool socket_get_msg(socket_int_t s, net_msg* ret);

// Отправить сообщение с телом
bool socket_send_msg(socket_int_t s, const net_msg& msg);
// Отправить сообщение без тела, только заголовок
bool socket_send_msg(socket_int_t s, const msg_types msg);

void socket_set_nonblock(const socket_int_t s);

// Флаг работы сетевого потока (только для сервера)
extern bool netd_started;

// Максимум клиентов
constexpr size_t clients_max = 10;

extern size_t clients_min;

constexpr size_t net_chunk_size = 1024;

// Флаг работы сервера
// Ждет подключения хотя бы одного клиента, затем отключения всех клиентов
bool run_server();

bool init_network();
void shutdown_network();

// Сетевой поток
void netd_server(socket_int_t sock_in, clients_list* clients, th_queue<net_msg>* queue, volatile bool* run);

#endif /* __NET_INT_H__ */