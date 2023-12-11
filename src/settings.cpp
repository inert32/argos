// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "settings.h"

std::filesystem::path verticies_file;

std::filesystem::path output_file = "output.list";

ipv4_t master_addr;

bool master_mode = false;

unsigned int chunk_elements = 100;

unsigned int threads_count = 0;

unsigned int port_server = 3700;

size_t clients_min = 0;

size_t clients_max = 10;

bool print_devel_info = (ARGOS_GIT_BRANCH == "main") ? false : true;