#ifndef __NET_DEF_H__
#define __NET_DEF_H__

/*
    Основные типы данных для работы с сетью
*/

#include <ostream>
#include <cstring>

extern unsigned int port_server;

constexpr size_t ipv4_ip_len = 16;
struct ipv4_t {
    char ip[ipv4_ip_len] = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
    size_t port = port_server;

    operator bool() const {
        return strcmp(ip, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0") != 0;
    }

    friend std::ostream& operator<<(std::ostream& os, const ipv4_t& p) {
        os << p.ip << ":" << p.port;
        return os;
    }

    void from_string(const std::string& str) {
        auto pos = str.find(':');
        std::string real_ip;
        if (pos != str.npos) {
            port = std::stoi(str.substr(pos + 1));
            real_ip = str.substr(0, pos);
        }
        else real_ip = str;
        const auto len = real_ip.length();

        for (size_t i = 0; i < len; i++) ip[i] = real_ip[i];
    }
    std::string to_string() {
        std::string ret;
        for (int i = 0; i < 16; i++) ret.push_back(ip[i]);
        return ret + ":" + std::to_string(port);
    }
};

enum class msg_types {
    BOTH_UNKNOWN = 0,

    SERVER_CLIENT_ACCEPT,
    SERVER_CLIENT_NOT_ACCEPT,
    SERVER_DATA,
    SERVER_DONE,
        
    CLIENT_CONNECT,
    CLIENT_CONNECT_CRYPT,
    CLIENT_GET_VECTORS,
    CLIENT_GET_TRIANGLE,
    CLIENT_DATA,
    CLIENT_DISCONNECT
};

struct net_msg {
    size_t peer_id = 0;
    msg_types type = msg_types::BOTH_UNKNOWN;
    size_t len = 0;
    char* data = nullptr;
};

#ifdef __linux__
typedef int socket_int_t;
#elif _WIN32
#include <WinSock2.h>
typedef SOCKET socket_int_t;
#endif /*__linux__*/

#endif /* __NET_DEF_H__ */
