#ifndef __CONVERT_H__
#define __CONVERT_H__

/*
    convert.h: Функции конвертации point,vec3,triangle <-> capsule_t*
*/

#include "network/net_base.h"

template <class T>
union conv_t {
    char side1[sizeof(T)];
    T side2;
};

// Перевод из вектора в массив char
capsule_t conv_vec_to_cap();

// Перевод из треугольника в массив char
capsule_t conv_tr_to_cap();

// Перевод из массива char в вектор
void conv_cap_to_vec(const capsule_t* from);

// Перевод в массива char в треугольник
void conv_cap_to_tr(const capsule_t* from);

#endif /* __CONVERT_H__ */