#include "sem_mng.hpp"
#include <string>
#include <stdexcept>
#ifdef _WIN32
#include <windows.h>
#else
#include <semaphore.h>
#include <fcntl.h>
#endif

SharedSemaphore::SharedSemaphore(const std::string& name)
: name_(name)
{
#ifdef _WIN32
    // КРИТИЧЕСКОЕ ИСПРАВЛЕНИЕ: именованный семафор для кроссплатформенной работы!
    hSemaphore_ = CreateSemaphoreA(
        nullptr,
        1,         // начальное значение
        1,         // максимальное значение
        name_.c_str()  // ← ИМЯ ОБЯЗАТЕЛЬНО!
    );
    if (!hSemaphore_) {
        throw std::runtime_error("CreateSemaphore failed, error: " +
            std::to_string(GetLastError()));
    }
#else
    sem_ = sem_open(name_.c_str(), O_CREAT, 0644, 1);
    if (sem_ == SEM_FAILED) {
        throw std::runtime_error("sem_open failed");
    }
#endif
}

SharedSemaphore::~SharedSemaphore() {
#ifdef _WIN32
    if (hSemaphore_) {
        CloseHandle(hSemaphore_);
    }
#else
    if (sem_ && sem_ != SEM_FAILED) {
        sem_close(sem_);
        // ВАЖНО: НЕ вызываем sem_unlink() здесь!
    }
#endif
}

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

bool SharedSemaphore::try_wait() {
#ifdef _WIN32
    DWORD res = WaitForSingleObject(hSemaphore_, 0); // 0 = немедленный возврат
    return (res == WAIT_OBJECT_0);
#else
    return (sem_trywait(sem_) == 0);
#endif
}