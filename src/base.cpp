// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <iostream>
#include <fstream>
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