// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <iostream>
#include <string.h>
#include "net.h"
#include "io.h"
#include "base.h"
#include "convert.h"

ipv4_t master_addr;

void ipv4_t::from_string(const std::string& str) {
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

std::string ipv4_t::to_string() {
    std::string ret;
    for (int i = 0; i < ipv4_ip_len; i++) ret.push_back(ip[i]);
    return ret + ":" + std::to_string(port);
}

bool ipv4_t::is_set() {
    bool ret = false;
    for (int i = 0; i < ipv4_ip_len; i++)
        if (ip[i] != '\0') { ret = true; break; }
    return ret;
}

void master_start(socket_int* socket) {
    std::cout << "MASTER MODE" << std::endl;
    try {
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