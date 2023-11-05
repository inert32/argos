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

void client_start();
void master_start();

#endif /* __NET_H__ */