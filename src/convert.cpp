// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "convert.h"

constexpr size_t point_size = sizeof(float) * 3;

const char* conv_point_to_char(const point* p) {
    char* ret = new char[point_size];
    conv_t<float> conv;
    unsigned long i = 0;

    conv.side2 = p->x;
    for (unsigned long byte = 0; byte < sizeof(float); byte++) ret[i++] = conv.side1[byte];

    conv.side2 = p->y;
    for (unsigned long byte = 0; byte < sizeof(float); byte++) ret[i++] = conv.side1[byte];

    conv.side2 = p->z;
    for (unsigned long byte = 0; byte < sizeof(float); byte++) ret[i++] = conv.side1[byte];

    return ret;
}

point conv_char_to_point(const char* c) {
    char* c_cut = new char[point_size];
    for (size_t i = 0; i < point_size; i++) c_cut[i] = c[i]; // Защита от переполнения буфера

    point ret;
    size_t i = 0;
    conv_t<float> conv;
    for (unsigned long byte = 0; byte < sizeof(float); byte++) conv.side1[byte] = c_cut[i++];
    ret.x = conv.side2;

    for (unsigned long byte = 0; byte < sizeof(float); byte++) conv.side1[byte] = c_cut[i++];
    ret.y = conv.side2;
    
    for (unsigned long byte = 0; byte < sizeof(float); byte++) conv.side1[byte] = c_cut[i++];
    ret.z = conv.side2;

    delete[] c_cut;
    return ret;
}

capsule_t conv_vec_to_cap(const vec3* from) {
    capsule_t ret;
    ret.len = point_size * 2; // Вектор = пара point
    ret.data = new char[ret.len];
    
    size_t i = 0;
    auto from_c = conv_point_to_char(&from->from);
    for (size_t byte = 0; byte < point_size; byte++) ret.data[i++] = from_c[byte];
    delete[] from_c;

    auto from_t = conv_point_to_char(&from->to);
    for (size_t byte = 0; byte < point_size; byte++) ret.data[i++] = from_t[byte];
    delete[] from_t;

    return ret;
}

capsule_t conv_tr_to_cap(const triangle* t) {
    capsule_t ret;
    ret.len = point_size * 3; // Треугольник = тройка point
    ret.data = new char[ret.len];
    
    size_t i = 0;
    auto from_a = conv_point_to_char(&t->A);
    for (size_t byte = 0; byte < point_size; byte++) ret.data[i++] = from_a[byte];
    delete[] from_a;

    auto from_b = conv_point_to_char(&t->B);
    for (size_t byte = 0; byte < point_size; byte++) ret.data[i++] = from_b[byte];
    delete[] from_b;

    auto from_c = conv_point_to_char(&t->C);
    for (size_t byte = 0; byte < point_size; byte++) ret.data[i++] = from_c[byte];
    delete[] from_c;
    
    return ret;
}

// Перевод из массива char в вектор
vec3 conv_cap_to_vec(const capsule_t* from) {
    unsigned long long int i = 0;
    const auto len = from->len;
    const auto data = from->data;

    vec3 ret;
    while (i < len) {
        char* curr_point = &data[i];

        ret.from = conv_char_to_point(curr_point);
        i += point_size;

        ret.to = conv_char_to_point(curr_point);
        i += point_size;
    }
    return ret;
}

// Перевод в массива char в треугольник
triangle conv_cap_to_tr(const capsule_t* from) {
    unsigned long long int i = 0;
    const auto len = from->len;
    const auto data = from->data;

    triangle ret;
    while (i < len) {
        char* curr_point = &data[i];

        ret.A = conv_char_to_point(curr_point);
        i += point_size;

        ret.B = conv_char_to_point(curr_point);
        i += point_size;

        ret.C = conv_char_to_point(curr_point);
        i += point_size;
    }
    return ret;
}