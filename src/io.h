#ifndef __IO_H__
#define __IO_H__

/* 
    Необходимые определения для загрузки и сохранения файлов
*/

#include <fstream>
#include <filesystem>
#include "base.h"
#include "network/net_base.h"

// Чтение файла вершин для работы с ними

// Интерфейс для чтения файла вершин (с диска или по сети, разных форматов)
class reader_base {
public:
	reader_base() = default;
	~reader_base() = default;

	// Получение треугольника из файла
	virtual bool get_next_triangle(triangle* ret) = 0;
	// Получение вектора из файла
	virtual bool get_next_vector(vec3* ret) = 0;

	// Проверка наличия треугольников в файле
	virtual bool have_triangles() const = 0;
	// Проверка наличия векторов в файле
	virtual bool have_vectors() const = 0;
};

// Собственный формат файла вершин
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

// Интерфейс для сохранения результатов
class saver_base {
public:
	saver_base() = default;
	~saver_base() = default;

	// Сохранение чанка данных
	virtual void save_tmp(volatile char** mat, const unsigned int count) = 0;

	// Объединение временного файла в выходной файл
	virtual void save_final() = 0;
};

// Сохранение результатов в файл
class saver_file : public saver_base {
public:
	saver_file();

	void save_tmp(volatile char** mat, const unsigned int count);
	void save_final();
private:
	std::ofstream file;
};

class saver_dummy : public saver_base {
public:
	saver_dummy() {};

	void save_tmp([[maybe_unused]] volatile char** mat, [[maybe_unused]] const unsigned int count) {}
	void save_final() {}
};

// Выбор класса saver в зависимости от режима работы
saver_base* select_saver(socket_int* socket);

// Выбор парсера в зависимости от режима работы и типа файла
reader_base* select_parser(socket_int* socket);

#endif /* __IO_H__ */