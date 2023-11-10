// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <iostream>
#include <thread>
#include <array>

#include "settings.h"
#include "th_queue.h"
#include "net.h"

unsigned int clients_max = 10;
unsigned int port_server = 3700;

void master_start(socket_t* socket) {
    std::cout << "Master mode" << std::endl;
    std::cout << "Listen port " << port_server << std::endl;

    std::vector<socket_t> clients_list;
    th_queue<net_msg> queue;
    volatile bool threads_run = true;

    std::thread netd(netd_server, socket, &clients_list, &queue, &threads_run);

    while (!netd_started) continue;

    std::cout << "Ready." << std::endl;
    while (true) {
        auto x = queue.take();
        if (!x) continue;
        auto msg = *x;
        auto send_to = clients_list[msg.peer_id];
        std::cout << "Type: " << (int)msg.type << std::endl;
        std::cout << "Len: " << msg.len << std::endl;
        switch (msg.type) {
            case msg_types::CLIENT_GET_VECTORS: {
                auto a = (char)msg_types::SERVER_DATA;
                send_to.send_msg(&a, 1);
                break;
            }
            case msg_types::CLIENT_GET_TRIANGLE: {
                auto a = (char)msg_types::SERVER_DONE;
                send_to.send_msg(&a, 1);
                break;
            }
            default: {
                auto a = (char)msg_types::BOTH_UNKNOWN;
                send_to.send_msg(&a, 1);
                break;
            }
        }
    }
    threads_run = false;
    while (netd_started) continue;
    netd.join();
}