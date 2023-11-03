// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "convert.h"
#include "base.h"

constexpr size_t point_size = sizeof(float) * 3;

const char* conv_point_to_char(const point* p) {
    char* ret = new char[point_size];
    conv_t conv;
    unsigned long i = 0;

    conv.side1 = p->x;
    for (unsigned long byte = 0; byte < sizeof(float); byte++) ret[i++] = conv.side2[byte];

    conv.side1 = p->y;
    for (unsigned long byte = 0; byte < sizeof(float); byte++) ret[i++] = conv.side2[byte];

    conv.side1 = p->z;
    for (unsigned long byte = 0; byte < sizeof(float); byte++) ret[i++] = conv.side2[byte];

    return ret;
}

point conv_char_to_point(const char* c) {
    char* c_cut = new char[point_size];
    strncpy(c_cut, c, point_size); // Защита от переполнения буфера

    point ret;
    size_t i = 0;
    conv_t conv;
    for (unsigned long byte = 0; byte < sizeof(float); byte++) conv.side2[byte] = c_cut[i++];
    ret.x = conv.side1;

    for (unsigned long byte = 0; byte < sizeof(float); byte++) conv.side2[byte] = c_cut[i++];
    ret.y = conv.side1;
    
    for (unsigned long byte = 0; byte < sizeof(float); byte++) conv.side2[byte] = c_cut[i++];
    ret.z = conv.side1;

    delete[] c_cut;
    return ret;
}

capsule_t conv_vec_to_cap() {
    capsule_t ret;
    const size_t size = vectors.size() * point_size * 2; // Вектор = пара point
    ret.len = size;
    ret.data = new char[size];
    
    size_t i = 0;
    for (auto &v : vectors) {
        auto from_c = conv_point_to_char(&v.from);
        for (size_t byte = 0; byte < point_size; byte++) ret.data[i++] = from_c[byte];
        delete[] from_c;

        auto from_t = conv_point_to_char(&v.to);
        for (size_t byte = 0; byte < point_size; byte++) ret.data[i++] = from_t[byte];
        delete[] from_t;
    }
    return ret;
}

capsule_t conv_tr_to_cap() {
    capsule_t ret;
    const size_t size = triangles.size() * point_size * 3; // Треугольник = тройка point
    ret.len = size;
    ret.data = new char[size];
    
    size_t i = 0;
    for (auto &t : triangles) {
        auto from_a = conv_point_to_char(&t.A);
        for (size_t byte = 0; byte < point_size; byte++) ret.data[i++] = from_a[byte];
        delete[] from_a;

        auto from_b = conv_point_to_char(&t.B);
        for (size_t byte = 0; byte < point_size; byte++) ret.data[i++] = from_b[byte];
        delete[] from_b;

        auto from_c = conv_point_to_char(&t.C);
        for (size_t byte = 0; byte < point_size; byte++) ret.data[i++] = from_c[byte];
        delete[] from_c;
    }
    return ret;
}

// Перевод из массива char в вектор
void conv_cap_to_vec(const capsule_t* from) {
    vectors.clear();

    unsigned long long int i = 0;
    const auto len = from->len;
    const auto data = from->data;

    while (i < len) {
        char* curr_point = &data[i];
        vec3 add;

        add.from = conv_char_to_point(curr_point);
        i += point_size;

        add.to = conv_char_to_point(curr_point);
        i += point_size;

        vectors.push_back(add);
    }
}

// Перевод в массива char в треугольник
void conv_cap_to_tr(const capsule_t* from) {
    triangles.clear();

    unsigned long long int i = 0;
    const auto len = from->len;
    const auto data = from->data;

    while (i < len) {
        char* curr_point = &data[i];
        triangle add;

        add.A = conv_char_to_point(curr_point);
        i += point_size;

        add.B = conv_char_to_point(curr_point);
        i += point_size;

        add.C = conv_char_to_point(curr_point);
        i += point_size;

        triangles.push_back(add);
    }
}