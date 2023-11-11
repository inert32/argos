#ifndef __NET_INT_H__
#define __NET_INT_H__

#include <vector>
#include "net_def.h"
#include "th_queue.h"

#ifdef __linux__
typedef int socket_int_t;
#endif /*__linux__*/

template <class T>
union conv_t {
    char side1[sizeof(T)];
    T side2;
};

class socket_t {
public:
    socket_t();
    socket_t(const socket_int_t old);
    ~socket_t();
    operator socket_int_t();

    void reset(const socket_int_t old);

    bool get_msg(net_msg* ret);

    bool send_msg(const net_msg& msg);
    bool send_msg(const msg_types msg);
    bool send_msg(const char* msg, const size_t msg_len);
private:
    socket_int_t s;
};

extern bool netd_started;

constexpr size_t clients_max = 10;
extern size_t clients_now;
extern std::array<socket_t*, clients_max> clients_list;

int setup_socket();
void close_socket(int socket);

void netd_server(socket_t* sock_in, th_queue<net_msg>* queue, volatile bool* run);

#endif /* __NET_INT_H__ */