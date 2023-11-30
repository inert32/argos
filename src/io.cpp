// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <iostream>
#include <map>
#include "settings.h"
#include "base.h"
#include "io.h"

std::set<size_t> clear_repeats(const std::string& str) {
    std::set<size_t> ret;
    size_t space_pos = str.find(' '), space_pos_old = 0;

    while (space_pos != std::string::npos) {
        while (str[space_pos] == ' ') space_pos++;
        const auto num = str.substr(space_pos_old, space_pos - space_pos_old);

        try {
            ret.insert(std::stoul(num));
        }
        catch (const std::exception&) {
            std::cout << "clear_repeats: failed to process raw num, positions: "
            << space_pos_old << " " << space_pos << ", full len: " << str.length() << std::endl;
            continue;
        }

        space_pos_old = space_pos;
        space_pos = str.find(' ', space_pos_old + 1);
    }
    return ret;
}

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

bool reader_argos::get_triangle(triangle* ret, const size_t id) {
    file.clear();
    file.seekg(triangles_start);

    size_t i = 0;
    std::string buf;
    while (i++ < id) std::getline(file, buf);

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

    file.seekg(triangles_current);
    return true;
}

void reader_argos::get_vectors() {
    file.clear();
    file.seekg(vectors_start);
    size_t id = 0;

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
        ret.id = id++;
        vectors.push_back(ret);
    }
}

bool reader_argos::have_triangles() const {
    return triangles_current < vectors_start;
}

saver_base::saver_base() {
    tmp_path = output_file.string() + ".tmp";
    final_path = output_file.string() + ".tmp-final";

    auto flags_tmp = ioflags;
    if (!std::filesystem::exists(tmp_path)) flags_tmp |= std::ios::trunc;
    tmp_file.open(tmp_path, flags_tmp);
    if (!tmp_file.good()) throw std::runtime_error("saver: Failed to open file " + tmp_path);

    auto flags_final = ioflags;
    if (!std::filesystem::exists(final_path)) flags_final |= std::ios::trunc;
    final_file.open(final_path, flags_final);
    if (!final_file.good()) throw std::runtime_error("saver: Failed to open file " + final_path);
}

saver_base::~saver_base() {
    tmp_file.close();
    final_file.close();

    if (!keep_tmp) {
        std::filesystem::remove(tmp_path);
        std::filesystem::remove(final_path);
    }
}

void saver_base::save(volatile char** mat, const unsigned int count) {
    const size_t vec_count = vectors.size();
    // Для каждого вектора указываем 
    for (size_t vec = 0; vec < vec_count; vec++) {
        auto& curr_vec = vectors[vec];
        tmp_file << curr_vec.id << ":";

        for (size_t tr = 0; tr < count; tr++)
            if (mat[vec][tr] == 1) {
                auto& t = triangles[tr];
                tmp_file << t.id << " ";
            }
        tmp_file << '\n';
    }
    tmp_file.flush();
}

void saver_base::compress() {
    std::cout << "Compressing output..." << std::endl;
    reset_file(tmp_file);
    reset_file(final_file);

    std::map<size_t, std::set<size_t>> data; // Словарь векторов и треугольников
    while (!tmp_file.eof()) {
        std::string buf;
        std::getline(tmp_file, buf);
        if (buf.empty() || tmp_file.eof()) break;

        // Получаем вектор
        const auto split = buf.find(':');
        const size_t vec_id = std::stoul(buf.substr(0, split));
        const std::string list = buf.substr(split + 1);

        // Добавляем данные в словарь
        data[vec_id].merge(clear_repeats(list));
    }

    // Сохраняем финальный результат в файл
    // Этот результат еще состоит из ids
    for (auto &[vec, tr] : data) {
        if (tr.empty()) continue; // Не записываем вектора, не попавшие по треугольникам

        final_file << vec << ':';
        for (auto &j : tr)
            final_file << j << " ";
        final_file << '\n';
    }

    const auto pre_size = std::filesystem::file_size(tmp_path);
    const auto post_size = std::filesystem::file_size(final_path);
    const double diff = (double)post_size / (double)pre_size * 100;

    std::cout << "compress: compressed bytes: " << pre_size << "->" << post_size << " (" << diff << "%)" << std::endl;
}

void saver_base::reset_file(std::fstream& file) {
    file.seekg(0);
    file.seekp(0);
    file.clear();
}

void saver_local::finalize() {
    std::cout << "Converting ids, this will take time..." << std::endl;
    // compress сохраняет результаты в виде id векторов и треугольников.
    // finalize должен перевести их в сами вектора и треугольники
    auto ref = select_parser(nullptr);
    reset_file(final_file);
    std::fstream out(output_file, std::ios::out);
    if (!out.good()) throw std::runtime_error("finalize: Failed to open file " + output_file.string());

    // Кэш треугольников
    std::map<size_t, std::string> map;

    while (!final_file.eof()) {
        std::string buf;
        std::getline(final_file, buf);
        if (buf.empty() || final_file.eof()) break;

        // Получаем вектор
        const auto split = buf.find(':');
        const size_t vec_id = std::stoul(buf.substr(0, split));
        const std::string list = buf.substr(split + 1);
        std::string out_line = vectors[vec_id].to_string() + ":";

        // Преобразуем строку в список id
        auto ids = clear_repeats(list);

        for (auto& i : ids) {
            if (map.find(i) == map.end()) { // Треугольника нет в кэше
                triangle tmp;
                ref->get_triangle(&tmp, i);
                map[i] += tmp.to_string() + ';';
            }
            out_line += map[i];
        }
        out_line.pop_back();
        out << out_line << '\n';
    }
}
