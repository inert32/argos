#ifndef __NET_BASE_H__
#define __NET_BASE_H__

/*
    net_base.h: Типы данных для сетевой подсистемы
*/
#include <ostream>
#include <cstddef>

enum class msg_types {
    BOTH_UNKNOWN = 0,

    SERVER_CLIENT_ACCEPT,
    SERVER_CLIENT_NOT_ACCEPT,
    SERVER_DATA,
    SERVER_DONE,
        
    CLIENT_CONNECT,
    CLIENT_CONNECT_CRYPT,
    CLIENT_GET_VECTORS,
    CLIENT_GET_TRIANGLES,
    CLIENT_DATA,
    CLIENT_DISCONNECT
};

constexpr auto ipv4_ip_len = 16;
struct ipv4_t {
    char ip[ipv4_ip_len] = "\0\0\0\0\0\0\0\0\0\0";
    int port = 0;

    friend std::ostream& operator<<(std::ostream& os, const ipv4_t& p) {
	    os << p.ip << ":" << p.port;
	    return os;
    }
    void from_string(const std::string& str);
    std::string to_string();
    bool is_set();
};

struct capsule_t {
    char* data = nullptr;
    size_t len = 0;
};

struct net_envelope {
    ipv4_t from;
    ipv4_t to;
    msg_types type = msg_types::BOTH_UNKNOWN;
    capsule_t msg_raw;
};

class socket_int;

#endif /* __NET_BASE_H__ */