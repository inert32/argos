#ifndef __POINT_H__
#define __POINT_H__

/* Определения точки и треугольника для вычислений */

class point {
public:
    point() = default;
    point(float _x, float _y, float _z) {
        x = _x; y = _y; z = _z;
    }
    point(const point& old) {
        x = old.x, y = old.y, z = old.z;
    }
    point(const point& from, const point& to) {
        x = to.x - from.x, y = to.y - from.y, z = to.z - from.z;
    }

    inline point operator+(point p) const {
        return { x + p.x, y + p.y, z + p.z };
    }
    inline point operator-(point p) const {
        return { x - p.x, y - p.y, z - p.z };
    }
    inline void operator=(point p) {
        x = p.x; y = p.y; z = p.z;
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