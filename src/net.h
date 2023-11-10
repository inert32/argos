#ifndef __NET_H__
#define __NET_H__

#include "net_def.h"
#include "net_int.h"
#include "io.h"

template <class T>
union conv_t {
    char side1[sizeof(T)];
    T side2;
};

class reader_network : public reader_base {
public:
    reader_network(socket_t* s);
	~reader_network();

	bool get_next_triangle(triangle* ret);
	void get_vectors();

	bool have_triangles() const;
private:
    socket_t* conn;
    bool server_have_triangles = true;
};

class saver_network : public saver_base {
public:
    saver_network(socket_t* s);
	~saver_network();

	void save_tmp(volatile char** mat, const unsigned int count);

	void save_final();
private:
    socket_t* conn;
    std::fstream file;
};

void master_start(socket_t* socket);

#endif /* __NET_H__ */