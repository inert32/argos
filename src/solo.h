#ifndef __SOLO_H__
#define __SOLO_H__

/* 
    Необходимые определения для одиночного режима
*/

struct thread_task {
    vec3 vec; // Вектор для обработки
    volatile char* ans = nullptr; // Строка в матрице ответов
};

// Начало работы в одиночном режиме
void solo_start();

#endif /* __SOLO_H__ */