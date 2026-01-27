#pragma once
#include <string>
#ifdef _WIN32
#include <windows.h>
#else
#include <semaphore.h>
#endif

class SharedSemaphore {
public:
    explicit SharedSemaphore(const std::string& name);
    SharedSemaphore(const SharedSemaphore&) = delete;
    SharedSemaphore& operator=(const SharedSemaphore&) = delete;
    ~SharedSemaphore();
    void wait();
    void signal();
    bool try_wait();  // ← НОВЫЙ МЕТОД: попытка захвата без блокировки
private:
    std::string name_;
#ifdef _WIN32
    void* hSemaphore_ = nullptr;
#else
    sem_t* sem_ = nullptr;
#endif
};