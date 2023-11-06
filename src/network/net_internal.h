#ifndef __NET_INTERNAL_H__
#define __NET_INTERNAL_H__

#include <string>
#include <arpa/inet.h>
#include "net_base.h"

/*
    net_internal.h: Платформо-зависимые классы и функции
    для работы с сетью
*/

// Определения
#ifdef __linux__
typedef int socket_t;
typedef sockaddr sockaddr_t;
typedef socklen_t sock_len;
#endif 

class socket_int {
public:
    socket_int(size_t port);
    ~socket_int();

    void get_msg(net_envelope* msg);
    void send_msg(net_envelope* msg);
private:
    socket_t in;
    sockaddr_t addr_local;

    std::string err_msg(const std::string& msg) const;
};

#endif /* __NET_INTERNAL_H__ */