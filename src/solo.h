#ifndef __SOLO_H__
#define __SOLO_H__

/* Необходимые определения для одиночного режима */
/* Парсер входного файла заданий, вычисления, запись результатов */

#include <fstream>
#include "point.h"

class parser {
public:
    parser();
    
    bool get_next_triangle(triangle* ret);
    bool get_next_vector(point* ret);
private:
    std::ifstream file;
    std::streampos triangles_start, vectors_start;
    std::streampos triangles_current, vectors_current;
};

void solo_start();

#endif /* __SOLO_H__ */