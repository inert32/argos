// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <vector>
#include <thread>
#include <iostream>
#include <map>

#include "../settings.h"
#include "net_int.h"

bool netd_started = false;

#ifdef _WIN32
bool init_network() {
    if (!(master_mode || master_addr)) return true; // Не запускаем сеть для одиночного режима

    WSADATA data;
    auto start = WSAStartup(MAKEWORD(2, 2), &data);
    if (start != 0) {
        std::cerr << "WSA: WSAStartup: " << start << std::endl;
        return false;
    }
    return true;
}
void shutdown_network() {
    if (!(master_mode || master_addr)) WSACleanup();
}
std::string make_err_msg(const std::string& msg) {
    auto err = WSAGetLastError();
    return msg + ": " + strerror(err) + " (" + std::to_string(err) + ")";
}
int plat_get_error() {
    return WSAGetLastError();
}
void socket_t::set_nonblock() {
    u_long mode = 1;
    ioctlsocket(s, FIONBIO, &mode);
}
void socket_t::kill() {
    closesocket(s);
}
bool socket_t::is_online() const {
    char c;
    auto l = recv(s, &c, sizeof(char), MSG_PEEK);
    if (l == 0) return false;
    return (l == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK) ? false : true;
}
int _mk_flag() {
    return (master_mode) ? 0 : MSG_WAITALL;
}
#elif __linux__
bool init_network() {
    return true;
}
void shutdown_network() {}
std::string make_err_msg(const std::string& msg) {
    auto err = errno;
    return msg + ": " + strerror(err) + " (" + std::to_string(err) + ")";
}
int plat_get_error() {
    return errno;
}
void socket_t::set_nonblock() {
    fcntl(s, F_SETFL, O_NONBLOCK);
}
void socket_t::kill() {
    close(s);
}
bool socket_t::is_online() const {
    char c;
    auto l = recv(s, &c, sizeof(char), MSG_PEEK);
    return (l > 0 || (errno == EAGAIN || errno == EWOULDBLOCK)) ? true : false;
}
int _mk_flag() { // Linux требует MSG_WAITALL для получения всех данных из recv
    return MSG_WAITALL;
}
#endif

ipv4_t::ipv4_t(const sockaddr src) {
    sockaddr_in data = *(sockaddr_in*)&src;
    port = ntohs(data.sin_port);

    const auto addr_raw = inet_ntoa(data.sin_addr);
    if (addr_raw != nullptr) std::memcpy(ip, addr_raw, ipv4_ip_len);
}

clients_list::clients_list() {
    for (size_t i = 0; i < clients_max; i++)
        list.push_back(nullptr);
}

clients_list::~clients_list() {
    for (auto &i : list) if (i != nullptr) i->kill();
}

bool clients_list::try_add(socket_t* s) {
    if (clients_count == clients_max) return false;

    for (size_t i = 0; i < clients_max; i++)
        if (list[i] == nullptr) {
            list[i] = s;
            list[i]->set_nonblock();

            clients_count++;
            if (max_clients_count < clients_count) max_clients_count = clients_count;
            wait_for_clients = false;

            return true;
        }
    return false;
}

void clients_list::remove(const size_t id) {
    list[id] = nullptr;
    clients_count--;
}

socket_t* clients_list::get(const size_t id) const {
    if (id >= clients_max) return nullptr;
    return list[id];
}

size_t clients_list::count() const {
    return clients_count;
}

size_t clients_list::max_count() const {
    return max_clients_count;
}

bool clients_list::run_server() const {
    return wait_for_clients || clients_count > 0;
}

std::vector<size_t> clients_list::poll_sockets(socket_t* conn_socket, bool* new_client) const {
    // Пары дескриптор - id сокета
    socket_int_t* socket_fd = new socket_int_t[clients_count];
    size_t* socket_id = new size_t[clients_count];

    // Заполняем пары
    size_t ind = 0;
    for (size_t i = 0; i < clients_max; i++)
        if (list[i] != nullptr) {
            socket_fd[ind] = list[i]->raw();
            socket_id[ind] = i;
            ind++;
        }

    auto poll_size = clients_count + 1;
    pollfd* poll_list = new pollfd[poll_size];

    // Используем poll() и для слежения за слушающим сокетом
    poll_list[0].fd = conn_socket->raw();
    poll_list[0].events = POLLIN;

    for (size_t i = 0; i < clients_count; i++) {
        poll_list[i + 1].fd = socket_fd[i];
        poll_list[i + 1].events = POLLIN;
    }

    auto poll_ret = poll(poll_list, poll_size, 1000);

    std::vector<size_t> ret;
    if (poll_ret == -1) std::cerr << make_err_msg("poll") << std::endl;

    if (poll_ret > 0) {
        if (poll_list[0].revents & POLLIN) *new_client = true;

        // Ищем сокеты с поступившими данными
        for (size_t i = 1; i < poll_size; i++)
            if (poll_list[i].revents & POLLIN) {
                ret.push_back(socket_id[i - 1]);
            }
    }

    delete[] poll_list;
    delete[] socket_fd;
    delete[] socket_id;
    return ret;
}

int calc_checksum(const net_msg& msg) {
    unsigned char sum = 0;
    for (size_t i = 0; i < msg.len; i++) sum ^= msg.data[i];
    return (int)sum;
}

void netd_server(socket_t* sock_in, clients_list* clients, th_queue<net_msg>* queue, volatile bool* run) {
    netd_started = true;

    while (*run == true) { //-V1044
        bool incoming = false;
        auto cl = clients->poll_sockets(sock_in, &incoming);

        // Подключение нового клиента
        if (incoming) {
            ipv4_t client_data;
            auto try_accept = sock_in->accept_conn(&client_data);
            if (try_accept != nullptr) {
                if (clients->try_add(try_accept)) {
                    std::cout << "Accepted client " << client_data << std::endl;
                    try_accept->send_msg(msg_types::SERVER_CLIENT_ACCEPT);
                }
                else try_accept->send_msg(msg_types::SERVER_CLIENT_NOT_ACCEPT);
            }
        }
        
        if (clients->max_count() < clients_min) continue;

        // Добавляем запросы подключенных клиентов в очередь
        for (auto &i : cl) {
            auto c = clients->get(i);
            if (c == nullptr) continue;
            if (!(c->is_online())) {
                clients->remove(i);
                continue;
            }

            net_msg ret;
            if (!c->get_msg(&ret)) continue;
            ret.peer_id = i;
            queue->add(ret);
        }
    }
    netd_started = false;
}

socket_t::socket_t() {
    s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) throw_err("Socket error");

    sockaddr_in addr;
    addr.sin_family = AF_INET;

    if (master_mode) {
        set_nonblock();
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_port = htons(port_server);
        if (bind(s, (sockaddr*)&addr, sizeof(addr)) < 0) throw_err("Bind error");
        if (listen(s, 1) == SOCKET_ERROR) throw_err("Listen fail");
    }
    else {
        addr.sin_addr.s_addr = inet_addr(master_addr.ip);
        addr.sin_port = htons(master_addr.port);
        if (connect(s, (sockaddr *)&addr, sizeof(addr)) < 0) throw_err("Connect error");

        net_msg check;
        bool ok = get_msg(&check);
        if (!ok || check.type != msg_types::SERVER_CLIENT_ACCEPT)
            throw std::runtime_error("Connect error: server declined connection.");
    }
}

socket_t::socket_t(const socket_int_t old) {
    s = old;
}

socket_int_t socket_t::raw() {
    return s;
}

socket_t* socket_t::accept_conn(ipv4_t* who) {
    sockaddr addr; socklen_t len = sizeof(sockaddr);

    auto try_accept = accept(s, &addr, &len);
    if (try_accept > 0) {
        if (who != nullptr) *who = ipv4_t(addr);
        return new socket_t(try_accept);
    }
    else return nullptr;
}

bool socket_t::get_msg(net_msg* ret) {
    header_t head;
    auto l = recv(s, (char*)&head, sizeof(header_t), 0);
    if (l < (signed)sizeof(header_t)) return false;

    ret->type = head.type;
    ret->len = head.raw_len - sizeof(header_t);

    const auto flag = _mk_flag();

    if (ret->len == 0) return true;
    // Копируем сообщение в ret
    ret->data = new char[ret->len];
    size_t bytes_got = 0;
    while (bytes_got < ret->len) {
        // Принимаем блоками по net_chunk_size байт
        // Вычисляем размер следующего блока
        const size_t bytes_remain = ret->len - bytes_got;
        const size_t block_len = (bytes_remain > net_chunk_size) ? net_chunk_size : bytes_remain;

        // Принимаем блок
        auto got = recv(s, &ret->data[bytes_got], block_len, flag); // MSG_WAITALL заблокирует поток пока мы получаем данные
        if (got == SOCKET_ERROR) {
            auto err = plat_get_error();
            if (err == plat_again || err == plat_wouldblock) bytes_got++; // строка 101 уменьшит bytes_got, компенсируем
            else {
                std::cout << "get_msg: " << strerror(err) << " (" << err << ")" << std::endl;
                break;
            }
        }
        bytes_got += got;
    }

    if (bytes_got < ret->len)
        std::cout << "get_msg: incomplete message got: " << bytes_got << " of " << ret->len << std::endl;

    return true;
}

bool socket_t::send_msg(const net_msg& msg) {
    header_t head;
    head.type = msg.type;
    head.raw_len += msg.len;
    const size_t raw_len = sizeof(header_t) + msg.len;

    char* msg_raw = new char[raw_len];
    std::memcpy(msg_raw, &head, sizeof(header_t));
    std::memcpy(&msg_raw[sizeof(header_t)], msg.data, msg.len);

    size_t bytes_send = 0;
    while (bytes_send < raw_len) {
        // Отправляем блоками по net_chunk_size байт
        // Вычисляем размер следующего блока
        const size_t bytes_remain = raw_len - bytes_send;
        const size_t block_len = (bytes_remain > net_chunk_size) ? net_chunk_size : bytes_remain;

        // Отправляем блок
        auto sent = send(s, &msg_raw[bytes_send], block_len, 0);
        if (sent == SOCKET_ERROR) break;
        bytes_send += sent;
    }
    if (bytes_send < raw_len)
        std::cout << "send_msg: incomplete message sent: " << bytes_send << " of " << raw_len << std::endl;

    delete[] msg_raw;
    return true;
}

bool socket_t::send_msg(const msg_types msg) {
    header_t head;
    head.type = msg;

    auto bytes_send = send(s, (char*)&head, sizeof(header_t), 0);
    if (bytes_send < (signed)sizeof(header_t))
        std::cout << "send_msg: incomplete message send (" << bytes_send << " of " << sizeof(msg_types) << ")" << std::endl;

    return true;
}