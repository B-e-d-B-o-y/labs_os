#pragma once
#ifdef _WIN32
#include <windows.h>
using thread_t = HANDLE;
#else
#include <pthread.h>
using thread_t = pthread_t;
#endif

// Создаёт поток: start_routine(void*) — функция, arg — её аргумент
int thread_create(thread_t* thread, void* (*start_routine)(void*), void* arg);
// Ждёт завершения потока
int thread_join(thread_t thread);
// Завершает текущий поток
void thread_exit();
// Проверяет, завершился ли процесс с PID
//  1 — процесс завершён
//  0 — процесс ещё жив
// -1 — ошибка
int check_process_finished(int pid);