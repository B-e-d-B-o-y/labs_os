#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#include <signal.h>
#include <sys/wait.h>
#endif
#include <stdexcept>   // для std::runtime_error
#include <string>      // для сообщений об ошибках
#include "thr.hpp"     // Подключаем заголовок

// Общее имя типа потока для двух платформ
#ifdef _WIN32
using thread_t = HANDLE;
#else
using thread_t = pthread_t;
#endif

// --- Реализация функций ---
int thread_create(thread_t* thread, void* (*start_routine)(void*), void* arg) {
#ifdef _WIN32
    // Создаём поток WinAPI
    *thread = CreateThread(
        nullptr,                           // атрибуты безопасности по умолчанию
        0,                                 // размер стека по умолчанию
        (LPTHREAD_START_ROUTINE)start_routine, // функция потока
        arg,                               // аргумент для функции
        0,                                 // флаги создания (0 — сразу стартовать)
        nullptr                            // id потока не нужен
    );
    return *thread == nullptr ? -1 : 0;    // 0 — успех, -1 — ошибка
#else
    // Создаём поток POSIX
    return pthread_create(thread, nullptr, start_routine, arg);
#endif
}

// Ожидание завершения потока
int thread_join(thread_t thread) {
#ifdef _WIN32
    // Ждём завершения потока
    WaitForSingleObject(thread, INFINITE);
    // Закрываем handle
    CloseHandle(thread);
    return 0;
#else
    // Ждём завершения потока POSIX
    return pthread_join(thread, nullptr);
#endif
}

// Завершение текущего потока
void thread_exit() {
#ifdef _WIN32
    ExitThread(0);         // Завершить поток с кодом 0
#else
    pthread_exit(nullptr); // Завершить поток POSIX
#endif
}

// Проверка, завершился ли процесс с заданным PID
// Возвращает:
//  1 — процесс завершён
//  0 — процесс ещё жив
// -1 — ошибка
int check_process_finished(int pid) {
#ifdef _WIN32
    // Открываем процесс по PID для запроса информации
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, static_cast<DWORD>(pid));
    if (hProcess == nullptr) {
        // Если процесса нет или нет прав — считаем, что он завершён
        return 1;
    }
    DWORD exitCode = 0;
    if (GetExitCodeProcess(hProcess, &exitCode)) {
        CloseHandle(hProcess);
        // STILL_ACTIVE означает, что процесс ещё работает
        return (exitCode == STILL_ACTIVE) ? 0 : 1;
    }
    // В случае ошибки получения кода выхода
    CloseHandle(hProcess);
    return -1;
#else
    int status = 0;
    // Не блокирующий waitpid: WNOHANG — не ждём, просто проверяем состояние
    int result = waitpid(pid, &status, WNOHANG);
    if (result == 0) {
        // Процесс ещё жив
        return 0;
    }
    if (result == pid) {
        // Процесс завершился
        return 1;
    }
    // Ошибка (например, нет такого процесса)
    return -1;
#endif
}