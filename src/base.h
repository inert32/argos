#ifndef __BASE_H__
#define __BASE_H__

/*
    base.h: Типы данных и функции, разделяемые между режимами
*/

#include <vector>
#include <fstream>
#include "point.h"

// Чтение файла вершин для работы с ними
class parser {
public:
    parser();
    
    // Получение треугольника из файла
    bool get_next_triangle(triangle* ret);
    // Получение вектора из файла
    bool get_next_vector(vec3* ret);

	bool have_triangles() const;
	bool have_vectors() const;
private:
    std::ifstream file;
    std::streampos triangles_start, vectors_start;
    std::streampos triangles_current, vectors_current;
};

// Сохранение результатов
class saver {
public:
	saver();
	void save_data(volatile char** mat, const unsigned int count);
private:
	std::ofstream file;
};

extern std::vector<triangle> triangles;
extern std::vector<vec3> vectors;

// Вычисление столкновений
char calc_collision(const triangle& t, const vec3 v);

// Сжатие выходного файла
void compress_output();

#endif /* __BASE_H__ */