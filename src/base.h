#ifndef __BASE_H__
#define __BASE_H__

/*
    Типы данных и функции, разделяемые между режимами
*/

#include <vector>

#include <ostream>

// Конвертер простых типов в массив байтов
template <class T>
union conv_t {
    char side1[sizeof(T)];
    T side2;
};

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
    std::string to_string() const {
        return "{ " + std::to_string(x) + " ; " + std::to_string(y) + " ; " + std::to_string(z) + " }";
    }

    // Машинный вывод
    std::string to_string_raw() const {
        return std::to_string(x) + " " + std::to_string(y) + " " + std::to_string(z);
    }

    void from_char(const char* src) {
        float parts[3];
        conv_t<float> conv;
        size_t offset = 0;

        for (int i = 0; i < 3; i++) {
            std::memcpy(conv.side1, &src[offset], sizeof(float));
            offset+=sizeof(float);
            parts[i] = conv.side2;
        }
        x = parts[0]; y = parts[1]; z = parts[2];
    }

    char* to_char() const {
        char* ret = new char[sizeof(point)];
        conv_t<float> conv;
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
    vec3() = default;
    vec3(const char* src) {
        conv_t<size_t> conv_id;
        std::memcpy(conv_id.side1, src, sizeof(size_t));
        id = conv_id.side2;

        size_t offset = sizeof(size_t);
        from.from_char(&src[offset]);
        offset+=sizeof(point);
        to.from_char(&src[offset]);
    }

    char* to_char() const {
        char* ret = new char[sizeof(vec3)];

        conv_t<size_t> conv_id;
        conv_id.side2 = id;
        std::memcpy(ret, conv_id.side1, sizeof(size_t));

        size_t offset = sizeof(size_t);
        std::memcpy(&ret[offset], from.to_char(), sizeof(point));
        offset+=sizeof(point);
        std::memcpy(&ret[offset], to.to_char(), sizeof(point));
        return ret;
    }
    std::string to_string() const {
        return from.to_string_raw() + ">" + to.to_string_raw();
    }

    size_t id = 0; // Порядковый номер в файле вершин
    point from, to;
};

class triangle {
public:
    triangle() = default;
    triangle(const char* src) {
        from_char(src);
    }
    void from_char(const char* src) {
        conv_t<size_t> conv_id;
        std::memcpy(conv_id.side1, src, sizeof(size_t));
        id = conv_id.side2;

        size_t offset = sizeof(size_t);
        A.from_char(&src[offset]);
        offset+=sizeof(point);
        B.from_char(&src[offset]);
        offset+=sizeof(point);
        C.from_char(&src[offset]);
    }

    char* to_char() const {
        char* ret = new char[sizeof(triangle)];

        conv_t<size_t> conv_id;
        conv_id.side2 = id;
        std::memcpy(ret, conv_id.side1, sizeof(size_t));

        size_t offset = sizeof(size_t);
        std::memcpy(&ret[offset], A.to_char(), sizeof(point));
        offset+=sizeof(point);
        std::memcpy(&ret[offset], B.to_char(), sizeof(point));
        offset+=sizeof(point);
        std::memcpy(&ret[offset], C.to_char(), sizeof(point));
        return ret;
    }
    std::string to_string() {
        return A.to_string_raw() + " " + B.to_string_raw() + " " + C.to_string_raw();
    }

    size_t id = 0; // Порядковый номер в файле вершин
    point A, B, C;
};

extern std::vector<triangle> triangles;
extern std::vector<vec3> vectors;

#endif /* __BASE_H__ */
