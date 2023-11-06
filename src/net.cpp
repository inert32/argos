// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <iostream>
#include <fstream>
#include <thread>
#include <cstring>
#include "th_queue.h"
#include "net.h"
#include "io.h"
#include "base.h"
#include "convert.h"
#include "network/clients.h"

void ipv4_t::from_string(const std::string& str) {
    auto pos = str.find(':');
    std::string real_ip;
    if (pos != str.npos) {
        port = std::stoi(str.substr(pos + 1));
        real_ip = str.substr(0, pos);
    }
    else real_ip = str;
    const auto len = real_ip.length();

    for (size_t i = 0; i < len; i++) ip[i] = real_ip[i];
}

std::string ipv4_t::to_string() {
    std::string ret;
    for (int i = 0; i < ipv4_ip_len; i++) ret.push_back(ip[i]);
    return ret + ":" + std::to_string(port);
}

bool ipv4_t::is_set() {
    bool ret = false;
    for (int i = 0; i < ipv4_ip_len; i++)
        if (ip[i] != '\0') { ret = true; break; }
    return ret;
}

void recv_thread(th_queue<net_envelope>* queue, socket_int* socket, volatile bool* run) {
    while (*run == true) {
        net_envelope msg;
        socket->get_msg(&msg);
        queue->add(msg);
    }
}

void master_start(socket_int* socket) {
    std::cout << "Master mode" << std::endl;
    try {
        auto parser = select_parser(nullptr);
        //auto saver = select_saver(nullptr);
        clients_list clients(15);

        th_queue<net_envelope> msg_queue;
        volatile bool run = true;
        auto recieve = new std::thread(recv_thread, &msg_queue, socket, &run);

        while (run) {
            auto task = msg_queue.take();
            if (!task) continue;
            net_envelope msg = *task, ans;

            // Клиент известен?
            if (!clients.in_array(msg.from.ip, nullptr)
                && msg.type == msg_types::CLIENT_CONNECT) {
                ans.to = msg.from;
                conv_t<size_t> conv;
                std::memcpy(conv.side1, msg.msg_raw.data, sizeof(size_t));

                ans.to.port = conv.side2;
                if (clients.add(ans.to)) {
                    std::cout << "Client add: " << ans.to << std::endl;
                    ans.type = msg_types::SERVER_CLIENT_ACCEPT;
                }
                else {
                    std::cout << "Client not add: " << ans.to << std::endl;
                    ans.type = msg_types::SERVER_CLIENT_NOT_ACCEPT;
                }

                try { socket->send_msg(&ans); }
                catch (const std::exception& e) {
                    std::cerr << e.what() << ", removing client." << std::endl;
                    clients.remove(ans.to.ip);
                }
                continue;
            }
            ans.to = clients.find_by_ip(msg.from.ip, nullptr);

            switch(msg.type) {
            case msg_types::CLIENT_GET_VECTORS: {
                std::cout << "Client " << ans.to << " requested vectors." << std::endl;
                vec3 load;
		        if (parser->get_next_vector(&load)) {
                    ans.type = msg_types::SERVER_DATA;
                    ans.msg_raw = conv_vec_to_cap(&load);
                }
                else {
                    std::cout << "... no more vectors found." << std::endl;
                    ans.type = msg_types::SERVER_DONE;
                }
                try { socket->send_msg(&ans); }
                catch (const std::exception& e) {
                    std::cerr << e.what() << std::endl;
                }
                break;
            }
            case msg_types::CLIENT_GET_TRIANGLES: {
                std::cout << "Client " << ans.to << " requested triangle." << std::endl;
                triangle ret;
                if (parser->get_next_triangle(&ret)) {
                    ans.type = msg_types::SERVER_DATA;
                    ans.msg_raw = conv_tr_to_cap(&ret);
                }
                else {
                    std::cout << "... no more triangles found." << std::endl;
                    ans.type = msg_types::SERVER_DONE;
                }
                try { socket->send_msg(&ans); }
                catch (const std::exception& e) {
                    std::cerr << e.what() << std::endl;
                }
                break;
            }
            case msg_types::CLIENT_DISCONNECT: {
                size_t ind;
                auto c = clients.find_by_ip(msg.from.ip, &ind);
                if (ind == (size_t)-1) break;

                std::cout << "Client " << c << " disconnect.";
                clients.remove(ind);
                break;
            }
            default:
                std::cout << "Unknown message type from client (" << (int)msg.type << ")" << std::endl;
                break;
            }
        }
        run = false;
        recieve->join();
        delete recieve;
    }
    catch (const std::runtime_error& e) {
		std::cerr << "err: " << e.what() << std::endl;
		return;
    }
}

reader_network::reader_network(socket_int* socket) {
    if (!master_addr.is_set()) throw std::runtime_error("reader_network: No master address provided");
    conn = socket;

    net_envelope start;
    start.to = master_addr;
    if (start.to.port == 0) start.to.port = conn_port_master;
    start.type = msg_types::CLIENT_CONNECT;

    // Передаем серверу порт для подключения к нам
    start.msg_raw.data = new char[sizeof(size_t)];
    start.msg_raw.len = sizeof(size_t);
    conv_t<size_t> conv;
    conv.side2 = conn_port_client;
    std::memcpy(start.msg_raw.data, conv.side1, sizeof(size_t));
    conn->send_msg(&start);

    net_envelope ans;
    std::cout << "reader_network: await for response..." << std::endl;
    conn->get_msg(&ans);
    if (ans.type == msg_types::SERVER_CLIENT_NOT_ACCEPT)
        throw std::runtime_error("reader_network: Master server refused to connect!");
    else if (ans.type != msg_types::SERVER_CLIENT_ACCEPT) 
        throw std::runtime_error("reader_network: Unknown command from master server");
    std::cout << "Successfully connected to master server." << std::endl;
}

reader_network::~reader_network() {
    std::cout << "Logging out..." << std::endl;
    net_envelope finish;
    finish.to = master_addr;
    finish.type = msg_types::CLIENT_DISCONNECT;
    conn->send_msg(&finish);
    conn = nullptr;
}

bool reader_network::get_next_triangle(triangle* ret) {
    std::cout << "reader_network: request triangle" << std::endl;
    net_envelope msg, ans;
    msg.to = master_addr;
    msg.type = msg_types::CLIENT_GET_TRIANGLES;
    conn->send_msg(&msg);

    conn->get_msg(&ans);
    if (ans.type == msg_types::SERVER_DATA) {
        *ret = conv_cap_to_tr(&ans.msg_raw);
        return true;
    }
    else {
        server_have_triangles = false;
        return false;
    }
}

bool reader_network::get_next_vector(vec3* ret) {
    std::cout << "reader_network: request vector" << std::endl;
    net_envelope msg, ans;
    msg.to = master_addr;
    msg.type = msg_types::CLIENT_GET_VECTORS;
    conn->send_msg(&msg);

    conn->get_msg(&ans);
    if (ans.type == msg_types::SERVER_DATA) {
        *ret = conv_cap_to_vec(&ans.msg_raw);
        return true;
    }
    else return false;
}

bool reader_network::have_triangles() const {
    return server_have_triangles;
}
bool reader_network::have_vectors() const {
    return true;
}

saver_network::saver_network(socket_int* socket) {
    file.open("output.tmp", std::ios::out | std::ios::app | std::ios::ate | std::ios::binary);
    conn = socket;
}
saver_network::~saver_network() {
    conn = nullptr;
}

void saver_network::save_tmp(volatile char** mat, const unsigned int count) {
    const size_t vec_count = vectors.size();
	for (size_t vec = 0; vec < vec_count; vec++) {
		auto curr_vec = vectors[vec];
		curr_vec.from.print_terse(file); file << ">";
		curr_vec.to.print_terse(file); file << ":";

		for (size_t tr = 0; tr < count; tr++)
			if (mat[vec][tr] == 1) {
				auto t = triangles[tr];
				t.A.print_terse(file); file << " ";
				t.B.print_terse(file); file << " ";
				t.C.print_terse(file); file << " ";
			}
		file << '\n';
	}
	file.flush();
}

void saver_network::save_final() {
    const auto len = std::filesystem::file_size("output.tmp");
    file.seekg(0);

    net_envelope msg;
    msg.to = master_addr;
    msg.type = msg_types::CLIENT_DATA;
    msg.msg_raw.len = len;
    msg.msg_raw.data = new char[len];

    for (size_t i = 0; i < len; i++) msg.msg_raw.data[i] = file.get();
    conn->send_msg(&msg);
}