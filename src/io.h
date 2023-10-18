#ifndef __IO_H__
#define __IO_H__

/* 
    Необходимые определения для загрузки и сохранения файлов
*/

#include <fstream>
#include <filesystem>
#include "base.h"

// Чтение файла вершин для работы с ними

// Интерфейс для чтения файла вершин (с диска или по сети, разных форматов)
class reader_base {
public:
	reader_base() = default;

	// Получение треугольника из файла
	virtual bool get_next_triangle(triangle* ret) = 0;
	// Получение вектора из файла
	virtual bool get_next_vector(vec3* ret) = 0;

	// Проверка наличия треугольников в файле
	virtual bool have_triangles() const = 0;
	// Проверка наличия векторов в файле
	virtual bool have_vectors() const = 0;
};

class reader_argos : public reader_base {
public:
	reader_argos();

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

reader_base* select_parser();

// Сохранение результатов
class tmp_saver {
public:
	tmp_saver();
	void save_data(volatile char** mat, const unsigned int count);
private:
	std::ofstream file;
};

// Сжатие выходного файла
void compress_output();

#endif /* __IO_H__ */