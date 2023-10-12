#ifndef __SOLO_H__
#define __SOLO_H__

/* Необходимые определения для одиночного режима */
/* Парсер входного файла заданий, вычисления, запись результатов */

// Файлы solo будет разделены на solo.{h,cpp} и base.{h,cpp}

#include <fstream>
#include <vector>
#include <queue>
#include <mutex>
#include <optional>
#include "point.h"
#include "th_queue.h"

// Чтение файла вершин для работы с ними
class parser {
public:
    parser();
    
    // Получение треугольника из файла
    bool get_next_triangle(triangle* ret);
    // Получение вектора из файла
    bool get_next_vector(vec3* ret);

	bool have_triangles();
	bool have_vectors();
private:
    std::ifstream file;
    std::streampos triangles_start, vectors_start;
    std::streampos triangles_current, vectors_current;
};

// Сохранение результатов
class saver {
public:
	saver();
	void save_data(volatile bool** mat, const unsigned int count);
private:
	std::ofstream file;
};

extern std::vector<triangle> triangles;
extern std::vector<vec3> vectors;

struct thread_task {
    vec3 vec; // Вектор для обработки
    volatile bool* ans = nullptr; // Строка в матрице ответов
};

// Начало работы в одиночном режиме
void solo_start();

// Вычисление столкновений
bool calc_collision(const triangle& t, const vec3 v);

// Сжатие выходного файла
void compress_output();

#endif /* __SOLO_H__ */