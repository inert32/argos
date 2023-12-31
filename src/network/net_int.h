#ifndef __NET_INT_H__
#define __NET_INT_H__

/*
    Внутренние функции работы с сетью
*/

#ifdef __linux__
#include <arpa/inet.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <fcntl.h>
#include "unistd.h"

typedef int socket_int_t;

constexpr int INVALID_SOCKET = -1;
constexpr int SOCKET_ERROR = -1;

// Платформо-зависимые коды ошибок
enum socket_err_codes {
    plat_again = EAGAIN,
    plat_wouldblock = EWOULDBLOCK
};

#elif _WIN32
#include <WinSock2.h>

typedef SOCKET socket_int_t;
typedef int socklen_t;

#define poll(fdArray, fds, timeout) WSAPoll(fdArray, fds, timeout)

// Платформо-зависимые коды ошибок
enum socket_err_codes {
    plat_again = 0,
    plat_wouldblock = WSAEWOULDBLOCK
};

#endif /*__linux__*/

#include <atomic>
#include "net_def.h"
#include "../th_queue.h"

std::string make_err_msg(const std::string& msg);
int plat_get_error();
#define throw_err(msg) throw std::runtime_error(make_err_msg(msg));

struct header_t {
    size_t raw_len = sizeof(header_t);
    msg_types type = msg_types::BOTH_UNKNOWN;
};

class clients_list {
public:
    clients_list();
    ~clients_list();

    bool try_add(socket_t* s);
    void remove(const size_t id);
    socket_t* get(const size_t id) const;

    size_t count() const;
    size_t max_count() const;

    std::vector<size_t> poll_sockets(socket_t* conn_socket, bool* new_client) const;

    bool run_server() const;
private:
    bool wait_for_clients = true;
    size_t clients_count = 0;
    size_t max_clients_count = 0;
    std::vector<socket_t*> list;
};

class socket_t {
public:
    socket_t();
    socket_t(const socket_int_t old);

    bool get_msg(net_msg* ret);

    bool send_msg(const net_msg& msg);
    bool send_msg(const msg_types msg);

    bool is_online() const;
    void set_nonblock();

    socket_t* accept_conn(ipv4_t* who);

    socket_int_t raw();

    void kill();
private:
    socket_int_t s;
};

class netd {
public:
    netd(socket_t* sock_in, clients_list* clients, th_queue<net_msg>* queue);

    void start();
    bool is_running() const { return running; }
    void stop();
private:
    void netd_main();
    // Разрешение на запуск потока
    std::atomic<bool> allow_run = false;
    // Флаг работы сетевого потока
    std::atomic<bool> running = false;

    socket_t* in = nullptr;
    clients_list* cl = nullptr;
    th_queue<net_msg>* q = nullptr;

    std::thread* t = nullptr;
};

int calc_checksum(const net_msg& msg);

constexpr size_t net_chunk_size = 1024;

bool init_network();
void shutdown_network();

#endif /* __NET_INT_H__ */