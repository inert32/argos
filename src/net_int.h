#ifndef __NET_INT_H__
#define __NET_INT_H__

#include "net_def.h"
#include "th_queue.h"

template <class T>
union conv_t {
    char side1[sizeof(T)];
    T side2;
};

socket_int_t socket_setup();
void socket_close(socket_int_t s);
bool socket_online(socket_int_t s);

bool socket_get_msg(socket_int_t s, net_msg* ret);

bool socket_send_msg(socket_int_t s, const net_msg& msg); 
bool socket_send_msg(socket_int_t s, const msg_types msg);

extern bool netd_started;

constexpr size_t clients_max = 10;
extern size_t clients_now;

bool run_server();

void netd_server(socket_int_t sock_in, socket_int_t** clients, th_queue<net_msg>* queue, volatile bool* run);

#endif /* __NET_INT_H__ */