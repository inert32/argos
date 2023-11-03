// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#ifdef __linux__

#include <iostream>

#include <vector>
#include <stdexcept>
#include <unistd.h>
#include <sys/socket.h>
#include <string.h>
#include "net_internal.h"

// Макрос для упрощения исключений
#define throw_err(msg) throw std::runtime_error(err_msg(msg))

socket_int::socket_int(size_t port) {
    in = socket(AF_INET, SOCK_STREAM, 0); // Открываем сокет
    if (in < 0) throw_err("Input socket fail");
    
    sockaddr_in addr; // Привязывем к сокету локальный адрес
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    addr_local = *(sockaddr*)&addr;
    addr_len = sizeof(addr);

    if (bind(in, &addr_local, addr_len) < 0) throw_err("Bind fail");
}

socket_int::~socket_int() {
    close(in);
}

void socket_int::get_msg(net_envelope* msg) {
    // Требует запуска в отдельном потоке
    if (listen(in, 1) == -1) throw_err("Listen fail");
    
    sockaddr_t client_data; socklen_t client_len;
    socket_t client = accept(in, &client_data, &client_len);
    if (client < 0) throw_err("Accept fail");

    // Сохраняем данные в временном контейнере
    std::vector<char> buf_str;
    while (true) {
        char buf;
        auto status = recv(client, &buf, 1, 0);
        if (status <= 0) break; // Чтение завершено
        buf_str.push_back(buf);
    }
    if (buf_str.size() == 0) return;
    auto bytes_read = buf_str.size();

    msg->from = get_ip(&client_data);
    msg->type = (msg_types)buf_str[0];
    msg->msg_raw.len = bytes_read - 1;
    msg->msg_raw.data = new char[bytes_read - 1];
    for (size_t i = 1; i < bytes_read; i++) msg->msg_raw.data[i - 1] = buf_str[i];
}

void socket_int::send_msg(net_envelope* msg) {
    socket_t out = socket(AF_INET, SOCK_STREAM, 0); // Открываем сокет на запись
    if (out < 0) throw_err("Output socket fail");
        
    sockaddr_in dest_addr; // Привязываем сокет к нужному адресу и порту
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(msg->to.port);
    dest_addr.sin_addr.s_addr = inet_addr(msg->to.ip);
        
    if (dest_addr.sin_addr.s_addr == INADDR_NONE) throw_err("Socket fail");
        
    auto dest_final = (sockaddr*)&dest_addr;
        
    auto status = connect(out, dest_final, addr_len); // Подключаемся
    if(status < 0) throw_err("Connect fail");

    capsule_t msg_raw;
    msg_raw.len = msg->msg_raw.len + 1;
    msg_raw.data = new char[msg_raw.len];
    msg_raw.data[0] = (char)msg->type;
    for (size_t i = 1; i < msg_raw.len; i++) msg_raw.data[i] = msg->msg_raw.data[i - 1];
        
    status = sendto(out, msg_raw.data, msg_raw.len, MSG_NOSIGNAL, dest_final, sizeof(dest_addr)); // Отправляем
    if(status <= 0) throw_err("Send fail");
    close(out);
}

ipv4_t socket_int::get_ip(const sockaddr_t* src) const {
    ipv4_t ret;
    sockaddr_in data = *(sockaddr_in*)(src == nullptr ? &addr_local : src);

    auto ip_raw = inet_ntoa(data.sin_addr);
    if (ip_raw) for (int i = 0; i < ipv4_ip_len; i++) ret.ip[i] = ip_raw[i];
    else for (int i = 0; i < ipv4_ip_len; i++) ret.ip[i] = '\0';

    ret.port = ntohs(data.sin_port);
    return ret;
}

std::string socket_int::err_msg(const std::string& msg) const {
    return msg + ": " + strerror(errno);
}

#endif /* __linux__ */