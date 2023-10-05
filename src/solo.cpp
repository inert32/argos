// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <fstream>
#include "common.h"
#include "solo.h"

parser::parser() {
    file.open(verticies_file);
    if (!file.good()) throw std::runtime_error("parser: Failed to open file " + filename.string());

    std::string header;
    std::getline(file, header);

    try {
        if (!(header[0] == 'V' && header[1] == ':')) 
            throw std::runtime_error("parser: " + filename.string() + " corrupt.");

        triangles_start = triangles_current = file.tellg();

        std::streampos vec_pos = std::stoull(header.substr(2));
        file.seekg(vec_pos);
        if (file.eof()) throw std::runtime_error("parser: " + filename.string() + " corrupt.");
        
        vectors_start = vectors_current = vec_pos;
    }
    catch (const std::exception&) {
        throw;
    }
}

bool parser::get_next_triangle(triangle* ret) {
    file.seekg(triangles_current);
    
    std::string buf;
    std::getline(file, buf);

    if (file.tellg() >= vectors_start) return false;

    float coord[9];
    size_t space = 0, next_space = 0;
    for (int i = 0; i < 9; i++) {
        next_space = buf.find(' ', space);
        coord[i] = std::stod(buf.substr(space, next_space));
        space = next_space + 1;
    }
    ret->A = {coord[0], coord[1], coord[2]};
    ret->B = {coord[3], coord[4], coord[5]};
    ret->C = {coord[6], coord[7], coord[8]};
    
    triangles_current = file.tellg();
    return true;
}

bool parser::get_next_vector(point* ret) {
    file.seekg(vectors_current);
    
    std::string buf;
    std::getline(file, buf);

    if (file.eof()) return false;

    point from, to; float coord[6];
    size_t space = 0, next_space = 0;
    for (int i = 0; i < 6; i++) {
        next_space = buf.find(' ', space);
        coord[i] = std::stod(buf.substr(space, next_space));
        space = next_space + 1;
    }
    ret->x = coord[3] - coord[0];
    ret->y = coord[4] - coord[1];
    ret->z = coord[5] - coord[2];

    vectors_current = file.tellg();
    return true;
}

void solo_start() {
    try {
        parser p;
    }
    catch (std::runtime_error& e) {

    }
}