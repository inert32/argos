#ifndef __POINT_H__
#define __POINT_H__

/* Определения точки и треугольника для вычислений */

#include <ostream>

class point {
public:
    point() = default;
    point(float _x, float _y, float _z) {
        x = _x; y = _y; z = _z;
    }
    point(const point& old) {
        x = old.x, y = old.y, z = old.z;
    }
    // Вычисление координат вектора
    point(const point& from, const point& to) {
        x = to.x - from.x, y = to.y - from.y, z = to.z - from.z;
    }

    inline point operator+(const point &p) const {
        return { x + p.x, y + p.y, z + p.z };
    }
    inline point operator-(const point& p) const {
        return { x - p.x, y - p.y, z - p.z };
    }
    inline void operator=(const point& p) {
        x = p.x; y = p.y; z = p.z;
    }
    // Скалярное произведение
    inline float operator*(const point& v) const {
        return x * v.x + y * v.y + z * v.z;
    }
    // Векторное произведение
    inline point operator|(const point& v) const {
        return { 
            y * v.z - z * v.y,
            z * v.x - x * v.z,
		    x * v.y - y * v.x 
        };
    }

    // Человекочитаемый вывод
	friend std::ostream& operator<<(std::ostream& os, const point& p) {
		os << "{ " << p.x << "; " << p.y << "; " << p.z << " }";
		return os;
	}
    // Машинный вывод
    void print_terse(std::ostream& os) const {
		os << x << " " << y << " " << z;
	}

    float x = 0.0, y = 0.0, z = 0.0;
};

struct vec3 {
	point from;
	point to;
};

struct triangle {
    point A;
    point B;
    point C;
};

#endif /* __POINT_H__ */