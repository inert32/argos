#ifndef __CLIENTS_H__
#define __CLIENTS_H__

#include "../settings.h"
#include "net_base.h"

class clients_list {
public:
    clients_list(const size_t count);
    ~clients_list();

    bool add(const char* ip, const size_t port);
    bool add(const ipv4_t& ip);

    void remove(const char* ip);
    void remove(const size_t index);

    bool in_array(const char* ip, size_t* index);

    ipv4_t find_by_ip(const char* ip, size_t* index);
private:
    size_t avaliable_slot();
    size_t find_ip(const char* ip);

    ipv4_t** array;

    size_t clients_current;
    size_t clients_max;
};

#endif /* __CLIENTS_H__ */