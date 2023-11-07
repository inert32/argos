// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <map>
#include "settings.h"
#include "point.h"
#include "base.h"
#include "io.h"

reader_argos::reader_argos() : reader_base() {
	file.open(verticies_file, std::ios::binary);
	if (!file.good()) throw std::runtime_error("parser: Failed to open file " + verticies_file.string());

	std::string header;
	std::getline(file, header);

	triangles_start = triangles_current = file.tellg();

	std::streampos vec_pos = std::stoull(header.substr(2));
	file.seekg(vec_pos);
	if (file.eof()) throw std::runtime_error("parser: " + verticies_file.string() + " corrupt.");

	vectors_start = vec_pos;
}

bool reader_argos::get_next_triangle(triangle* ret) {
	file.clear();
	file.seekg(triangles_current);

	std::string buf;
	std::getline(file, buf);

	if (file.tellg() >= vectors_start || buf.empty()) return false;

	float coord[9];
	size_t space = 0, next_space = 0;
	for (int i = 0; i < 9; i++) {
		next_space = buf.find(' ', space);
		coord[i] = std::stod(buf.substr(space, next_space));
		space = next_space + 1;
	}
	ret->A = { coord[0], coord[1], coord[2] };
	ret->B = { coord[3], coord[4], coord[5] };
	ret->C = { coord[6], coord[7], coord[8] };

	triangles_current = file.tellg();
	return true;
}

void reader_argos::get_vectors() {
	file.clear();
	file.seekg(vectors_start);

	while (!file.eof()) {
		std::string buf;
		std::getline(file, buf);

		if (file.eof() || buf.empty()) return;

		float coord[6];
		size_t space = 0, next_space = 0;
		for (int i = 0; i < 6; i++) {
			next_space = buf.find(' ', space);
			coord[i] = std::stod(buf.substr(space, next_space));
			space = next_space + 1;
		}
		vec3 ret;
		ret.from.x = coord[0];
		ret.from.y = coord[1];
		ret.from.z = coord[2];
		ret.to.x = coord[3];
		ret.to.y = coord[4];
		ret.to.z = coord[5];
		vectors.push_back(ret);
	}
}

bool reader_argos::have_triangles() const {
	return triangles_current < vectors_start;
}

reader_base* select_parser() {
	std::ifstream file(verticies_file, std::ios::binary);
	if (!file.good()) throw std::runtime_error("select_parser: Failed to open file " + output_file.string());

	// Проверяем заголовки
	std::string header;
	std::getline(file, header);
	file.close();

	if (header[0] == 'V' && header[1] == ':') {
		auto ret = new reader_argos();
		return ret;
	}
	else // Неизвестный формат 
		throw std::runtime_error("select_parser: " + verticies_file.string() + ": unknown format.");
}

saver_file::saver_file() : saver_base() {
	file.open(output_file.string() + ".tmp", std::ios::app | std::ios::ate | std::ios::binary);
	if (!file.good()) throw std::runtime_error("saver: Failed to open file " + output_file.string() + ".tmp");
}

void saver_file::save_tmp(volatile char** mat, const unsigned int count) {
	const size_t vec_count = vectors.size();
	// Для каждого вектора указываем 
	for (size_t vec = 0; vec < vec_count; vec++) {
		auto curr_vec = vectors[vec];
		curr_vec.from.print_terse(file); file << ">";
		curr_vec.to.print_terse(file); file << ":";

		for (size_t tr = 0; tr < count; tr++)
			if (mat[vec][tr] == 1) {
				auto t = triangles[tr];
				t.A.print_terse(file); file << " ";
				t.B.print_terse(file); file << " ";
				t.C.print_terse(file); file << " ";
			}
		file << '\n';
	}
	file.flush();
}

void saver_file::save_final() {
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

	for (auto &i : data)
		if (!i.second.empty()) out << i.first << ':' << i.second << std::endl;

	base.close();
	out.close();
	std::filesystem::remove(base_path);
}

saver_base* select_saver() {
	auto ret = new saver_file;
	return ret;
}