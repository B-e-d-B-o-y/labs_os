#include <string>       // std::string для имён объектов
#include <stdexcept>    // std::runtime_error для ошибок
#include <cstring>      // std::memset для обнуления структуры
#include "mem_shr.hpp"  // Подключаем заголовок 
#include "sem_mng.hpp"  // Подключаем заголовок для SharedSemaphore

#ifdef _WIN32
#include <windows.h>    // WinAPI: CreateFileMapping, MapViewOfFile, CreateSemaphore и т.д.
#else
#include <sys/mman.h>   // shm_open, mmap, munmap
#include <sys/types.h>  // типы для POSIX вызовов
#include <sys/stat.h>   // права доступа (0666 и т.п.)
#include <fcntl.h>      // константы O_CREAT, O_RDWR
#include <unistd.h>     // close
#include <semaphore.h>  // POSIX семафоры
#include <errno.h>      // errno для ошибок POSIX
#endif


SharedMemory::SharedMemory(const std::string& name)
    : name_(name)
{
#ifdef _WIN32
    // Создаём/открываем объект отображаемого файла в памяти
    hMapFile_ = CreateFileMappingA(
        INVALID_HANDLE_VALUE,                 // Используем анонимную память, не реальный файл
        nullptr,                              // Атрибуты безопасности по умолчанию
        PAGE_READWRITE,                       // Чтение/запись
        0,                                    // Старшая часть размера (0, т.к. размер маленький)
        static_cast<DWORD>(sizeof(SharedMemoryData)), // Размер области
        name_.c_str()                         // Имя объекта (одно и то же во всех процессах)
    );
    if (!hMapFile_) {
        // Если не получилось создать/открыть — бросаем исключение
        throw std::runtime_error("CreateFileMapping failed, error: " +
                                 std::to_string(GetLastError()));
    }
    // Отображаем объект в адресное пространство процесса
    void* p = MapViewOfFile(
        hMapFile_,                // Дескриптор объекта отображения
        FILE_MAP_ALL_ACCESS,      // Права чтения/записи
        0,                        // Смещение по старшей части
        0,                        // Смещение по младшей части
        sizeof(SharedMemoryData)  // Размер отображения
    );
    if (!p) {
        CloseHandle(hMapFile_);   // Чистим ресурс
        throw std::runtime_error("MapViewOfFile failed, error: " +
                                 std::to_string(GetLastError()));
    }
    // Сохраняем указатель на разделяемую структуру
    data_ = static_cast<SharedMemoryData*>(p);
#else   // POSIX-вариант
    // Открываем/создаём объект POSIX shared memory
    int shm_fd = shm_open(name_.c_str(), O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        throw std::runtime_error("shm_open failed");
    }
    // Устанавливаем размер сегмента под нашу структуру
    if (ftruncate(shm_fd, sizeof(SharedMemoryData)) == -1) {
        close(shm_fd);
        throw std::runtime_error("ftruncate failed");
    }
    // Отображаем shared memory в адресное пространство процесса
    void* p = mmap(nullptr,
                   sizeof(SharedMemoryData),
                   PROT_READ | PROT_WRITE,   // Чтение/запись
                   MAP_SHARED,               // Общая память между процессами
                   shm_fd,
                   0);
    close(shm_fd);                            // Дескриптор fd больше не нужен
    if (p == MAP_FAILED) {
        throw std::runtime_error("mmap failed");
    }
    // Сохраняем указатель на разделяемую структуру
    data_ = static_cast<SharedMemoryData*>(p);
#endif
}

// Деструктор освобождает ресурсы
SharedMemory::~SharedMemory() {
#ifdef _WIN32
    if (data_) {
        UnmapViewOfFile(data_);              // Отсоединяем отображение
    }
    if (hMapFile_) {
        CloseHandle(hMapFile_);              // Закрываем объект отображения
    }
#else
    if (data_) {
        munmap(data_, sizeof(SharedMemoryData)); // Отсоединяем shared memory
    }
    
#endif
}

// Доступ к "сырым" данным
SharedMemoryData* SharedMemory::get() { return data_; }
const SharedMemoryData* SharedMemory::get() const { return data_; }

// Обнулить всю структуру в shared memory (например, при первом запуске)
void SharedMemory::set_zero() {
    if (!data_) return;
    std::memset(data_, 0, sizeof(SharedMemoryData));
}