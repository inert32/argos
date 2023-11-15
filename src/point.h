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
    point(const char* raw) {
        from_char(raw);
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

    void from_char(const char* src) {
        float parts[3];
        union { char side1[sizeof(float)]; float side2; } conv;
        size_t offset = 0;

        for (int i = 0; i < 3; i++) {
            std::memcpy(conv.side1, &src[offset], sizeof(float));
            offset+=sizeof(float);
            parts[i] = conv.side2;
        }
        x = parts[0]; y = parts[1]; z = parts[2];
    }

    char* to_char() {
        char* ret = new char[sizeof(point)];
        union { char side1[sizeof(float)]; float side2; } conv;
        size_t offset = 0;

        conv.side2 = x;
        std::memcpy(&ret[offset], conv.side1, sizeof(float));
        offset+=sizeof(float);

        conv.side2 = y;
        std::memcpy(&ret[offset], conv.side1, sizeof(float));
        offset+=sizeof(float);

        conv.side2 = z;
        std::memcpy(&ret[offset], conv.side1, sizeof(float));

        return ret;
    }

    float x = 0.0, y = 0.0, z = 0.0;
};

class vec3 {
public:
    char* to_char() {
        char* ret = new char[sizeof(vec3)];
        size_t offset = 0;

        std::memcpy(&ret[offset], from.to_char(), sizeof(point));
        offset+=sizeof(point);
        std::memcpy(&ret[offset], to.to_char(), sizeof(point));

        return ret;
    }

	point from;
	point to;
};

class triangle {
public:
    char* to_char() {
        char* ret = new char[sizeof(triangle)];
        size_t offset = 0;

        std::memcpy(&ret[offset], A.to_char(), sizeof(point));
        offset+=sizeof(point);
        std::memcpy(&ret[offset], B.to_char(), sizeof(point));
        offset+=sizeof(point);
        std::memcpy(&ret[offset], C.to_char(), sizeof(point));

        return ret;
    }

    point A;
    point B;
    point C;
};

#endif /* __POINT_H__ */