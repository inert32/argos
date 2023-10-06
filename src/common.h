#ifndef __COMMON_H__
#define __COMMON_H__

/* common.h : Содержит общие настройки приложения,
   а так же общие типы данных
*/

#include <filesystem>

constexpr char ARGOS_VERSION[] = "v0.0.3";

// Путь до исходного файла заданий
extern std::filesystem::path verticies_file;
// Путь до выходного файла
extern std::filesystem::path output_file;

// Адрес мастер-сервера. Не реализовано, do not use.
// Используем int пока не появится сетевая подсистема
extern unsigned int ipv4addr;

// Ражим мастер-сервера. Не реализовано
extern bool master_mode;

// Количество треугольников для расчета
extern unsigned int chunk_elements;

#endif /* __COMMON_H__ */
