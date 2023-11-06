#ifndef __NET_H__
#define __NET_H__

#include "settings.h"
#include "network/net_base.h"
#include "network/net_internal.h"
#include "base.h"
#include "io.h"

/*
    net.h: Платформо-независимые классы и функции
    для работы с сетью
*/

class reader_network : public reader_base {
public:
    reader_network(socket_int* socket);
    ~reader_network();

	bool get_next_triangle(triangle* ret);
	bool get_next_vector(vec3* ret);

	bool have_triangles() const;
	bool have_vectors() const;
private:
    socket_int* conn;
    bool server_have_triangles = true;
};
class saver_network : public saver_base {
public:
    saver_network(socket_int* socket);
    ~saver_network();

	void save_tmp(volatile char** mat, const unsigned int count); // Промежуточные результаты сохраняются на диск
	void save_final(); // Окончательные передаются мастеру
private:
    socket_int* conn;
    std::fstream file;
};

void master_start(socket_int* socket);

#endif /* __NET_H__ */