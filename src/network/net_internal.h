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
typedef unsigned int socklen_t;
#endif 

class socket_int {
public:
    socket_int(size_t port);
    ~socket_int();

    void get_msg(net_envelope* msg);
    void send_msg(net_envelope* msg);
    ipv4_t get_ip(const sockaddr_t* src) const;
private:
    socket_t in;
    sockaddr_t addr_local;
    socklen_t addr_len;

    std::string err_msg(const std::string& msg) const;
};

#endif /* __NET_INTERNAL_H__ */