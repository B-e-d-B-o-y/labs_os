// D:\labs_os\Lab3\sem_mng.hpp
#pragma once
#include <string>
#ifdef _WIN32
#include <windows.h>
#else
#include <semaphore.h>
#endif

// Класс семафора для синхронизации доступа к shared memory
class SharedSemaphore {
public:
    explicit SharedSemaphore(const std::string& name);
    SharedSemaphore(const SharedSemaphore&) = delete;
    SharedSemaphore& operator=(const SharedSemaphore&) = delete;
    ~SharedSemaphore();
    void wait();   // захват
    void signal(); // освобождение

private:
    std::string name_;
#ifdef _WIN32
    void* hSemaphore_ = nullptr; // HANDLE
#else
    sem_t* sem_ = nullptr;
#endif
};