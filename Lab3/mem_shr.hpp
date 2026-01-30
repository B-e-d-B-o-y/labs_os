#pragma once
#include <string>

// Структура, лежащая в shared memory
struct SharedMemoryData {
    int counter;
    int is_initialized; // Флаг инициализации (0 - не инициализирован, 1 - инициализирован)
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
    SharedMemoryData* data_ = nullptr; // Указатель на структуру данных
#ifdef _WIN32
    void* hMapFile_ = nullptr; // HANDLE
#endif
};

// Подключаем заголовок с определением SharedSemaphore ДО inline-функций
#include "sem_mng.hpp"

// Объявления inline-функций
int get_counter(SharedMemoryData* memory, SharedSemaphore& sem);
void set_counter(SharedMemoryData* memory, SharedSemaphore& sem, int value);
void increment_counter(SharedMemoryData* memory, SharedSemaphore& sem);
void set_zero_shared_memory(SharedMemoryData* memory, SharedSemaphore& sem);
bool is_initialized(SharedMemoryData* memory, SharedSemaphore& sem);
void mark_initialized(SharedMemoryData* memory, SharedSemaphore& sem);

inline int get_counter(SharedMemoryData* memory, SharedSemaphore& sem) {
    sem.wait();                     // Вход в критическую секцию
    int value = memory->counter;    // Читаем значение
    sem.signal();                   // Выход из критической секции
    return value;
}

inline void set_counter(SharedMemoryData* memory, SharedSemaphore& sem, int value) {
    sem.wait();
    memory->counter = value;        // Записываем новое значение
    sem.signal();
}

inline void increment_counter(SharedMemoryData* memory, SharedSemaphore& sem) {
    sem.wait();
    ++memory->counter;              // Инкремент
    sem.signal();
}

inline void set_zero_shared_memory(SharedMemoryData* memory, SharedSemaphore& sem) {
    sem.wait();
    std::memset(memory, 0, sizeof(SharedMemoryData)); // Обнуляем всё
    memory->is_initialized = 1;    // Помечаем как инициализированную
    sem.signal();
}

inline bool is_initialized(SharedMemoryData* memory, SharedSemaphore& sem) {
    sem.wait();
    bool initialized = (memory->is_initialized == 1);
    sem.signal();
    return initialized;
}

inline void mark_initialized(SharedMemoryData* memory, SharedSemaphore& sem) {
    sem.wait();
    memory->is_initialized = 1;
    sem.signal();
}