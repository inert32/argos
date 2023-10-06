#ifndef __SOLO_H__
#define __SOLO_H__

/* Необходимые определения для одиночного режима */
/* Парсер входного файла заданий, вычисления, запись результатов */

#include <fstream>
#include <vector>
#include <map>
#include "point.h"

// Чтение файла вершин для работы с ними
class parser {
public:
    parser();
    
    bool get_next_triangle(triangle* ret);
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
	void save_data(bool** mat, const unsigned int count);
private:
	std::ofstream file;
};

extern std::vector<triangle> triangles;
extern std::vector<vec3> vectors;

// Начало работы в одиночном режиме
void solo_start();

// Вычисление столкновений
bool calc_collision(const triangle& t, const vec3 v);

// Сжатие выходного файла
void compress_output();

#endif /* __SOLO_H__ */