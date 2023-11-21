#ifndef __NET_H__
#define __NET_H__

/*
    Платформо-независимые функции работы с сетью
*/

#include "network/net_def.h"
#include "network/net_int.h"
#include "io.h"

// Класс загрузки файла вершин по сети
class reader_network : public reader_base {
public:
    reader_network(const socket_int_t s);
    ~reader_network() = default;

    bool get_next_triangle(triangle* ret);
    bool get_triangle(triangle* ret, const size_t id);
    void get_vectors();

    bool have_triangles() const;
private:
    socket_int_t conn;
    bool server_have_triangles = true;
};

// Класс передачи результатов серверу
class saver_network : public saver_base {
public:
    saver_network(const socket_int_t s);
    ~saver_network() = default;

    void save_tmp(volatile char** mat, const unsigned int count);
    void save_final() {}

    void convert_ids() {}
private:
    socket_int_t conn;
};

void master_start(socket_int_t socket);

#endif /* __NET_H__ */
