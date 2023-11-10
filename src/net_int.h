#ifndef __NET_INT_H__
#define __NET_INT_H__

#include <vector>
#include "net_def.h"
#include "th_queue.h"

#ifdef __linux__
typedef int socket_int_t;
#endif /*__linux__*/

class socket_t {
public:
    socket_t();
    socket_t(socket_int_t old);
    ~socket_t();
    operator socket_int_t();

    bool get_msg(net_msg* ret);
    bool send_msg(const net_msg& msg);
    bool send_msg(const char* msg, const size_t& msg_len);
private:
    socket_int_t s;
};

extern bool netd_started;

int setup_socket();
void close_socket(int socket);

void netd_server(socket_t* sock_in, std::vector<socket_t>* sock_list, th_queue<net_msg>* queue, volatile bool* run);

#endif /* __NET_INT_H__ */