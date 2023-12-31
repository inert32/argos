#ifndef __COMMON_H__
#define __COMMON_H__

/* 
    Содержит общие настройки приложения,
    а так же общие типы данных
*/

#include <string_view>
#include <filesystem>
#include "network/net_def.h"

extern std::string_view ARGOS_VERSION;
extern std::string_view ARGOS_GIT_BRANCH;
extern std::string_view ARGOS_GIT_DATA;

// Путь до исходного файла заданий
extern std::filesystem::path verticies_file;
// Путь до выходного файла
extern std::filesystem::path output_file;

// Адрес мастер-сервера. Не реализовано, do not use.
// Используем int пока не появится сетевая подсистема
extern ipv4_t master_addr;

// Ражим мастер-сервера. Не реализовано
extern bool master_mode;

// Количество треугольников для расчета
extern unsigned int chunk_elements;

// Количество потоков для расчета
extern unsigned int threads_count;

// Сервер будет слушать этот порт
extern unsigned int port_server;

// Максимум клиентов
extern size_t clients_max;

// Минимум клиентов для начала работы
extern size_t clients_min;

extern bool print_devel_info;

#endif /* __COMMON_H__ */