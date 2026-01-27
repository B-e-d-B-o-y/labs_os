#pragma once
#include <string>
struct SharedMemoryData {
    int counter;
};
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
    void* hMapFile_ = nullptr;
#endif
};
#include "sem_mng.hpp"
inline int get_counter(SharedMemoryData* memory, SharedSemaphore& sem) {
    sem.wait();
    int value = memory->counter;
    sem.signal();
    return value;
}
inline void set_counter(SharedMemoryData* memory, SharedSemaphore& sem, int value) {
    sem.wait();
    memory->counter = value;
    sem.signal();
}
inline void increment_counter(SharedMemoryData* memory, SharedSemaphore& sem) {
    sem.wait();
    ++memory->counter;
    sem.signal();
}
inline void set_zero_shared_memory(SharedMemoryData* memory, SharedSemaphore& sem) {
    sem.wait();
    memory->counter = 0;
    sem.signal();
}