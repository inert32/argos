#ifndef __BASE_H__
#define __BASE_H__

/*
    base.h: Типы данных и функции, разделяемые между режимами
*/

#include <vector>
#include "point.h"

extern std::vector<triangle> triangles;
extern std::vector<vec3> vectors;

// Вычисление столкновений
char calc_collision(const triangle& t, const vec3 v);

#endif /* __BASE_H__ */