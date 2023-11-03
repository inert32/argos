// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <iostream>
#include <string.h>
#include "net.h"
#include "io.h"
#include "base.h"
#include "convert.h"

ipv4_t master_addr;

bool ipv4_isset(const ipv4_t* ip) {
    bool ret = false;
    for (int i = 0; i < ipv4_ip_len; i++)
        if (ip->ip[i] != '\0') { ret = true; break; }
    return ret;
}

ipv4_t string_to_ipv4(const std::string& str) {
    ipv4_t ret;

    auto pos = str.find(':');
    std::string real_ip;
    if (pos != str.npos) {
        ret.port = std::stoi(str.substr(pos + 1));
        real_ip = str.substr(0, pos);
    }
    else real_ip = str;
    const auto len = real_ip.length();

    for (size_t i = 0; i < len; i++) ret.ip[i] = real_ip[i];
    return ret;
}

std::string ipv4_to_string(const ipv4_t& ip) {
    std::string ret;
    for (int i = 0; i < ipv4_ip_len; i++) ret.push_back(ip.ip[i]);
    return ret + ":" + std::to_string(ip.port);
}

void client_start() {
    std::cout << "Client mode" << std::endl;
    std::cout << "Connect to: " << ipv4_to_string(master_addr);
    try {
        auto socket = new socket_int(3457);

        net_envelope msg;
        msg.type = msg_types::CLIENT_CONNECT;
        msg.to = master_addr;
        socket->send_msg(&msg);
        delete socket;
    }
    catch (const std::runtime_error& e) {
		std::cerr << "err: " << e.what() << std::endl;
		return;
    }

}
void master_start() {
    std::cout << "MASTER MODE" << std::endl;
    try {
        auto socket = new socket_int(3456);
        bool run = true;
        while (run) {
            net_envelope msg;
            socket->get_msg(&msg);
            std::cout << "Got message." << std::endl;
            std::cout << "From: " << msg.from << std::endl;
            std::cout << "Type: " << (int)msg.type << std::endl;
            std::cout << "Len: " << msg.msg_raw.len << std::endl;
        }
    }
    catch (const std::runtime_error& e) {
		std::cerr << "err: " << e.what() << std::endl;
		return;
    }
}