#ifndef __NET_H__
#define __NET_H__

#include "settings.h"
#include "network/net_base.h"
#include "base.h"
#include "network/net_internal.h"
#include "th_queue.h"

/*
    net.h: Платформо-независимые классы и функции
    для работы с сетью
*/

bool ipv4_isset(const ipv4_t* ip);
ipv4_t string_to_ipv4(const std::string& str);
std::string ipv4_to_string(const ipv4_t& ip);

void client_start();
void master_start();

#endif /* __NET_H__ */