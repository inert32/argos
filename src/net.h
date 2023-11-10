#ifndef __NET_H__
#define __NET_H__

#include "net_def.h"

int setup_socket();
void close_socket(int socket);

void master_start(int* sokcet);

#endif /* __NET_H__ */