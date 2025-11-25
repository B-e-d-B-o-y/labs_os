#pragma once

#include <string>
#include <stdexcept>

#ifdef _WIN32
#include <windows.h>
#include <limits.h>
#else
#include <semaphore.h>
#include <fcntl.h>
#include <errno.h>
#include <system_error>
#endif

class Semaphore {
public:
    // Создать (или открыть) именованный семафор с начальным значением value
    Semaphore(const std::string& name, unsigned int value)
        : name_(name)
    {
#ifdef _WIN32
        handle_ = CreateSemaphoreA(
            nullptr,              // атрибуты безопасности по умолчанию
            static_cast<LONG>(value), // начальное значение
            LONG_MAX,             // максимальное значение
            name_.empty() ? nullptr : name_.c_str() // имя (или безымянный)
        );
        if (handle_ == nullptr) {
            throw std::runtime_error("Failed to create semaphore, error: " +
                                     std::to_string(GetLastError()));
        }
#else
        handle_ = sem_open(name_.c_str(), O_CREAT, 0644, value);
        if (handle_ == SEM_FAILED) {
            throw std::system_error(errno, std::generic_category(),
                                    "Failed to create semaphore");
        }
#endif
    }

    // Запрещаем копирование, разрешаем перемещение при необходимости
    Semaphore(const Semaphore&) = delete;
    Semaphore& operator=(const Semaphore&) = delete;

    // Деструктор освобождает ресурсы
    ~Semaphore() {
#ifdef _WIN32
        if (handle_) {
            CloseHandle(handle_);
        }
#else
        if (handle_ && handle_ != SEM_FAILED) {
            sem_close(handle_);
            // sem_unlink можно вызывать один раз в "главном" процессе,
            // здесь вызовем только если имя непустое
            if (!name_.empty()) {
                sem_unlink(name_.c_str());
            }
        }
#endif
    }

    // Ожидание семафора (эквивалент semaphore_wait)
    void wait() {
#ifdef _WIN32
        DWORD result = WaitForSingleObject(handle_, INFINITE);
        if (result != WAIT_OBJECT_0) {
            throw std::runtime_error("Failed to wait for semaphore, error: " +
                                     std::to_string(GetLastError()));
        }
#else
        if (sem_wait(handle_) == -1) {
            throw std::system_error(errno, std::generic_category(),
                                    "Failed to wait for semaphore");
        }
#endif
    }

    // Сигнал семафора (эквивалент semaphore_signal / sem_post)
    void signal() {
#ifdef _WIN32
        if (!ReleaseSemaphore(handle_, 1, nullptr)) {
            throw std::runtime_error("Failed to signal semaphore, error: " +
                                     std::to_string(GetLastError()));
        }
#else
        if (sem_post(handle_) == -1) {
            throw std::system_error(errno, std::generic_category(),
                                    "Failed to signal semaphore");
        }
#endif
    }

private:
    std::string name_;
#ifdef _WIN32
    HANDLE handle_ = nullptr;
#else
    sem_t* handle_ = nullptr;
#endif
};