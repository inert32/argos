#ifndef __COMMON_H__
#define __COMMON_H__

/* 
    Содержит общие настройки приложения,
    а так же общие типы данных
*/

#include <filesystem>
#include "net_def.h"

constexpr char ARGOS_VERSION[] = "v0.1.4";

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

#endif /* __COMMON_H__ */
