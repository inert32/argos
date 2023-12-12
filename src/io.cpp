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
    const size_t len = str.length();

    while (space_pos_old < len) {
        while (str[space_pos] == ' ') space_pos++;
        const auto num = str.substr(space_pos_old, space_pos - space_pos_old);

        try { ret.insert(std::stoul(num)); }
        catch (const std::exception&) {
            std::cout << "clear_repeats: failed to process raw num, positions: "
            << space_pos_old << " " << space_pos << ", full len: " << str.length() << std::endl;
        }

        space_pos_old = space_pos;
        space_pos = str.find(' ', space_pos_old + 1);
        if (space_pos == str.npos) space_pos = len;
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
    ret->id = triangle_id++;

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
    ret->id = i;

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

void saver_base::save(volatile char** mat, const unsigned int count, full_map* map) {
    const size_t vec_count = vectors.size();

    for (size_t vec = 0; vec < vec_count; vec++)
        for (size_t tr = 0; tr < count; tr++)
            if (mat[vec][tr] == 1) map->at(vec).insert(triangles[tr].id);
}

saver_local::saver_local() : saver_base() {
    out.open(output_file, std::ios::out);
    if (!out.good()) throw std::runtime_error("finalize: Failed to open file " + output_file.string());
}

saver_local::~saver_local() {
    out.close();
}

void saver_local::finalize(const full_map& m) {
    std::cout << "Saving to " << output_file << "..." << std::endl;
    auto ref = select_parser(nullptr);

    // Кэш треугольников
    std::map<size_t, std::string> cache;
    triangle load;
    while (ref->get_next_triangle(&load)) cache[load.id] = load.to_string();

    for (auto &i : m) {
        const size_t vec = i.first;

        out << vectors[vec].to_string() << ":";

        for (auto &j : i.second)
            out << cache[j] << " ";
        out << "\n";
    }
    delete ref;
}
