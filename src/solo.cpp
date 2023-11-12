// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <iostream>
#include <map>
#include <thread>

#include "settings.h"
#include "th_queue.h"
#include "base.h"
#include "io.h"
#include "net.h"

// Флаг остановки потоков
volatile bool stop = false;

struct thread_task {
    vec3 vec; // Вектор для обработки
    volatile char* ans = nullptr; // Строка в матрице ответов
};

// Вычисления в отдельном потоке
void worker_main(th_queue<thread_task>* src) {
    while (!stop) {
        auto task = src->take();
        if (task) {
            auto ans = task->ans;
            const auto vec_c = task->vec;
            const auto count = triangles.size();
            for (size_t i = 0; i < count; i++)
                ans[i] = calc_collision(triangles[i], vec_c);
        }
        else std::this_thread::yield();
    }
}

// Проверка матрицы на заполненность
bool check_matr(volatile char** mat, size_t* ind) {
    const auto vec_count = vectors.size();
    const auto count = triangles.size();
    bool ret = false;
    for (size_t i = 0; i < vec_count; i++)
        if (mat[i][count - 1] == 2) {
            ret = true;
            *ind = i;
            break;
        }
    return ret;
}

void solo_start(socket_int_t* socket) {
    try {
        if (socket != nullptr) std::cout << "Client mode" << std::endl;
        else std::cout << "Solo mode" << std::endl;

        auto p = select_parser(socket);
		auto s = select_saver(socket);

        // Загружаем векторы
        p->get_vectors();
        std::cout << "Loaded " << vectors.size() << " vectors" << std::endl;

        // Создаем матрицу ответов
        const size_t vec_count = vectors.size();
		volatile char** ans_matr = new volatile char*[vec_count];
        for (size_t i = 0; i < vec_count; i++) {
            ans_matr[i] = new volatile char[chunk_elements];
            for (size_t j = 0; j < chunk_elements; j++) ans_matr[i][j] = 2;
        }

        // Создаем потоки и очередь заданий
        th_queue<thread_task> *tasks = new th_queue<thread_task>;
        std::vector<std::thread> workers;
        for (size_t i = 0; i < threads_count; i++) workers.emplace_back(worker_main, tasks);

        size_t chunks_count = 0;
        while (p->have_triangles()) {
            // Загружаем треугольники
            triangle load2; size_t count = 0;
	    	while (count < chunk_elements && p->get_next_triangle(&load2)) {
		    	triangles.push_back(load2);
			    count++;
		    }
            if (count == 0) break;

            // Помечаем матрицу как непроверенную перед вычислением новых треугольников
            // До 0f4eb59d77c8a4a2d1a3efc87a03d9307a0b2260 матрица пересоздавалась 
            // каждую итерацию цикла, теперь метку о заполнении строки нужно ставить вручную.
            for (size_t i = 0; i < vec_count; i++) {
	    		ans_matr[i] = new volatile char[count];
		    	for (size_t j = 0; j < count; j++) ans_matr[i][j] = 2;
    		}

            std::cout << "Calculating triangles: " << 1 + chunks_count
                        << " of " << chunks_count + count << std::endl;

            // Ждем завершения вычислений
            for (size_t i = 0; i < vec_count; i++) {
                thread_task tmp = {vectors[i], ans_matr[i]};
                tasks->add(tmp);
            }
            while (tasks->count() > 0) std::this_thread::yield();

            // Проверяем на выполнение вычислений
            // При неизвестных условиях цикл в worker_main может не дойти до конца матрицы.
            // Поэтому проходим по правому концу матрицы и пересчитываем.
            bool errors = true;
            while (errors) {
                size_t ind = 0;
                errors = check_matr(ans_matr, &ind);
                if (errors) {
                    thread_task tmp = {vectors[ind], ans_matr[ind]};
                    tasks->add(tmp);
                }
                while (tasks->count() > 0) std::this_thread::yield();
            }

            // Сохраняем
            s->save_tmp(ans_matr, count);

            // Очищаем данные для следующей партии треугольников
            triangles.clear();
            chunks_count+=count;
        }
        // Останавливаем потоки
        stop = true;
        for (size_t i = 0; i < threads_count; i++) workers[i].join();

        for (size_t i = 0; i < vec_count; i++) delete[] ans_matr[i];
        delete[] ans_matr;
        // При количестве треугольников больше чем chunks_elements
        // вектора в выходном файле будут повторяться. Исправляем.
		std::cout << "Compressing output..." << std::endl;
        s->save_final();
    }
    catch (const std::runtime_error& e) {
		std::cerr << "err: " << e.what() << std::endl;
		return;
    }
}
