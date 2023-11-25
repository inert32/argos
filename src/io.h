#ifndef __IO_H__
#define __IO_H__

/* 
    Необходимые определения для загрузки и сохранения файлов
*/

#include <fstream>
#include <filesystem>
#include <set>
#include "base.h"
#include "network/net_def.h"

// Чтение файла вершин для работы с ними

// Интерфейс для чтения файла вершин (с диска или по сети, разных форматов)
class reader_base {
public:
    reader_base() = default;
    ~reader_base() = default;

    // Получение треугольника из файла
    virtual bool get_next_triangle(triangle* ret) = 0;
    virtual bool get_triangle(triangle* ret, const size_t id) = 0;
    // Получение векторов из файла
    virtual void get_vectors() = 0;

    // Проверка наличия треугольников в файле
    virtual bool have_triangles() const = 0;
};

// Собственный формат файла вершин
class reader_argos : public reader_base {
public:
    reader_argos();

    // Получение треугольника из файла
    bool get_next_triangle(triangle* ret);
    bool get_triangle(triangle* ret, const size_t id);
    // Получение векторов из файла
    void get_vectors();

    bool have_triangles() const;
private:
    std::ifstream file;
    std::streampos triangles_start, vectors_start;
    std::streampos triangles_current;
};

// Интерфейс для сохранения результатов
class saver_base {
public:
    saver_base();
    ~saver_base();

    // Сохранение чанка данных
    void save(volatile char** mat, const unsigned int count);

    // Объединение временного файла
    void compress();

    virtual void finalize() = 0;

protected:
    void reset_file(std::fstream& file);
    // Временный файл до удаления дубликатов
    std::string tmp_path;
    std::fstream tmp_file;

    // Временный файл после объединения
    std::string final_path;
    std::fstream final_file;
};

// Сохранение результатов в файл
class saver_local : public saver_base {
public:
    // Перевод идентификаторов в треугольники и векторы
    void finalize();
};

class saver_dummy : public saver_base {
public:
    saver_dummy() = default;

    void save([[maybe_unused]] volatile char** mat, [[maybe_unused]] const unsigned int count) {}
    void compress() {}
    void finalize() {}
};

// Выбор парсера в зависимости от режима работы и типа файла
reader_base* select_parser(socket_t* s);

// Выбор класса saver в зависимости от режима работы
saver_base* select_saver(socket_t* s);

// Перевод str -> std::set
std::set<size_t> clear_repeats(const std::string& str);

// Флаги для std::fstream
constexpr auto ioflags = std::ios::in | std::ios::out | std::ios::binary;

#endif /* __IO_H__ */
