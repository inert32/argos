// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <iostream>
#include <fstream>
#include <map>

#include "settings.h"
#include "base.h"

constexpr float EPS = 1e-6;

std::vector<triangle> triangles;
std::vector<vec3> vectors;

char calc_collision(const triangle& t, const vec3 v) {
	// Представляем треугольник как лучи от одной точки
	const point dir(v.from, v.to);
	point e1(t.A, t.B), e2(t.A, t.C);

	// Вычисление вектора нормали к плоскости
	point pvec = dir | e2;
	float det = e1 * pvec;

	// Луч параллелен плоскости
	if (det < EPS && det > -EPS) return 0;

	float inv_det = 1 / det;
	point tvec(t.A, v.from);

	float coord_u = tvec * pvec * inv_det;
	if (coord_u < 0.0 || coord_u > 1.0) return 0;

	point qvec = tvec | e1;
	float coord_v = dir * qvec * inv_det;
	if (coord_v < 0 || coord_u + coord_v > 1) return 0;

	float coord_t = e2 * qvec * inv_det;
	return (coord_t > EPS) ? 1 : 0;
}

void compress_output() {
    std::cout << "Compressing output..." << std::endl;

    const auto base_path = output_file.string() + ".tmp";
    std::ifstream base(base_path); // Выходной файл до сжатия
    if (!base.good()) throw std::runtime_error("compress_output: is " + base_path + " not exists?");
    std::ofstream out(output_file); // Выходной файл после сжатия
    if (!out.good()) throw std::runtime_error("compress_output: failed to open " + output_file.string());

    std::map<std::string, std::string> data; // Словарь векторов и треугольников
    while (!base.eof()) {
        std::string buf;
        std::getline(base, buf);
        if (buf.empty() || base.eof()) break;

        // Получаем вектор
        const auto split = buf.find(':');
        const std::string vec = buf.substr(0, split);
        const std::string list = buf.substr(split + 1);

        if (data.find(vec) == data.end()) // Если вектор не найден
            data[vec] = list; // Создаем пару вектор-треугольники
        else
            data[vec] = data[vec] + list + ' '; // Добавляем новые треугольники к старым
    }

    for (auto &i : data) out << i.first << ':' << i.second << std::endl;
    
    base.close();
    out.close();
    std::filesystem::remove(base_path);
}