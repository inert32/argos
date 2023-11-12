#ifndef __NET_INT_H__
#define __NET_INT_H__

/*
    Внутренние функции работы с сетью
*/

#include "net_def.h"
#include "th_queue.h"

// Конвертер простых типов в массив байтов
template <class T>
union conv_t {
    char side1[sizeof(T)];
    T side2;
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

// Флаг работы сетевого потока (только для сервера)
extern bool netd_started;

// Максимум клиентов
constexpr size_t clients_max = 10;
// Текущее число клиентов
extern size_t clients_now;

// Флаг работы сервера
// Ждет подключения хотя бы одного клиента, затем отключения всех клиентов
bool run_server();

// Сетевой поток
void netd_server(socket_int_t sock_in, socket_int_t** clients, th_queue<net_msg>* queue, volatile bool* run);

#endif /* __NET_INT_H__ */