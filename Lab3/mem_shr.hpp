#pragma once

#include <string>

// Структура, лежащая в shared memory
struct SharedMemoryData {
    int counter;
};

// Обёртка над разделяемой памятью
class SharedMemory {
public:
    explicit SharedMemory(const std::string& name);
    SharedMemory(const SharedMemory&) = delete;
    SharedMemory& operator=(const SharedMemory&) = delete;
    ~SharedMemory();

    SharedMemoryData* get();
    const SharedMemoryData* get() const;

    void set_zero();

private:
    std::string name_;
    SharedMemoryData* data_ = nullptr;

#ifdef _WIN32
    void* hMapFile_ = nullptr; // HANDLE, в .c++ сделаешь правильный тип
#endif
};

// Вспомогательные функции работы с счётчиком под защитой семафора
class SharedSemaphore; // объявляем вперёд, определён в sem_mng.hpp

int get_counter(SharedMemoryData* memory, SharedSemaphore& sem);
void set_counter(SharedMemoryData* memory, SharedSemaphore& sem, int value);
void increment_counter(SharedMemoryData* memory, SharedSemaphore& sem);
void set_zero_shared_memory(SharedMemoryData* memory, SharedSemaphore& sem);