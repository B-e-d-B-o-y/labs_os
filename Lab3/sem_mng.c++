// D:\labs_os\Lab3\sem_mng.c++ (Linux version)
#include "sem_mng.hpp"  // Подключаем заголовок
#include <string>
#include <stdexcept>
#ifdef _WIN32
#include <windows.h>
#else
#include <semaphore.h>
#include <fcntl.h>
#include <errno.h>
#endif

// --- Реализация методов класса SharedSemaphore ---
SharedSemaphore::SharedSemaphore(const std::string& name)
    : name_(name)
{
#ifdef _WIN32
    // Создаём (или открываем) семафор
    hSemaphore_ = CreateSemaphoreA(
        nullptr,   // Атрибуты безопасности по умолчанию
        1,         // Начальное значение (разрешён 1 доступ)
        1,         // Максимальное значение
        nullptr    // Можно задать имя, если нужен именованный IPC-семафор
    );
    if (!hSemaphore_) {
        throw std::runtime_error("CreateSemaphore failed, error: " +
                                 std::to_string(GetLastError()));
    }
#else
    // Открываем/создаём именованный POSIX семафор с начальным значением 1
    sem_ = sem_open(name_.c_str(), O_CREAT, 0644, 1);
    if (sem_ == SEM_FAILED) {
        throw std::runtime_error("sem_open failed");
    }
#endif
}

// Семафор тоже нельзя копировать (это ресурс ОС)
// SharedSemaphore(const SharedSemaphore&) = delete;
// SharedSemaphore& operator=(const SharedSemaphore&) = delete;

SharedSemaphore::~SharedSemaphore() {
#ifdef _WIN32
    if (hSemaphore_) {
        CloseHandle(hSemaphore_); // Закрываем handle
    }
#else
    if (sem_ && sem_ != SEM_FAILED) {
        sem_close(sem_);          // Закрываем семафор
        sem_unlink(name_.c_str()); // Удаляем имя семафора из системы (один раз)
    }
#endif
}

// Захват семафора (вход в критическую секцию)
void SharedSemaphore::wait() {
#ifdef _WIN32
    DWORD res = WaitForSingleObject(hSemaphore_, INFINITE);
    if (res != WAIT_OBJECT_0) {
        throw std::runtime_error("WaitForSingleObject failed, error: " +
                                 std::to_string(GetLastError()));
    }
#else
    if (sem_wait(sem_) == -1) {
        throw std::runtime_error("sem_wait failed");
    }
#endif
}

// Освобождение семафора (выход из критической секции)
void SharedSemaphore::signal() {
#ifdef _WIN32
    if (!ReleaseSemaphore(hSemaphore_, 1, nullptr)) {
        throw std::runtime_error("ReleaseSemaphore failed, error: " +
                                 std::to_string(GetLastError()));
    }
#else
    if (sem_post(sem_) == -1) {
        throw std::runtime_error("sem_post failed");
    }
#endif
}