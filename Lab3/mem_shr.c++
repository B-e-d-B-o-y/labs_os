#pragma once  // Защита от повторного включения заголовка (аналог include guard)

#include <string>       // std::string для имён объектов
#include <stdexcept>    // std::runtime_error для ошибок
#include <cstring>      // std::memset для обнуления структуры

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

// Структура, которая реально лежит в разделяемой памяти
// Сюда могут добавляться поля при необходимости (пока только счётчик)
struct SharedMemoryData {
    int counter;
};

// Класс-обёртка над разделяемой памятью
class SharedMemory {
public:
    // Конструктор создаёт или открывает область совместно используемой памяти
    explicit SharedMemory(const std::string& name)
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

    // Копирование запрещаем, чтобы случайно не дублировать владение ресурсами
    SharedMemory(const SharedMemory&) = delete;
    SharedMemory& operator=(const SharedMemory&) = delete;

    // Деструктор освобождает ресурсы
    ~SharedMemory() {
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
        // shm_unlink можно вызывать отдельно в "главном" процессе по необходимости
#endif
    }

    // Доступ к "сырым" данным
    SharedMemoryData* get() { return data_; }
    const SharedMemoryData* get() const { return data_; }

    // Обнулить всю структуру в shared memory (например, при первом запуске)
    void set_zero() {
        if (!data_) return;
        std::memset(data_, 0, sizeof(SharedMemoryData));
    }

private:
    std::string name_;             // Имя сегмента shared memory
    SharedMemoryData* data_ = nullptr;  // Указатель на разделяемую структуру

#ifdef _WIN32
    HANDLE hMapFile_ = nullptr;    // Дескриптор объекта отображаемой памяти
#endif
};


// Класс для семафора, синхронизирующего доступ к SharedMemoryData
class SharedSemaphore {
public:
    explicit SharedSemaphore(const std::string& name)
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
    SharedSemaphore(const SharedSemaphore&) = delete;
    SharedSemaphore& operator=(const SharedSemaphore&) = delete;

    ~SharedSemaphore() {
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
    void wait() {
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
    void signal() {
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

private:
    std::string name_;   // Имя семафора (для POSIX; для Windows можно тоже использовать)

#ifdef _WIN32
    HANDLE hSemaphore_ = nullptr;  // WinAPI handle семафора
#else
    sem_t* sem_ = nullptr;         // Указатель на POSIX-семафор
#endif
};


// Удобные inline-функции работы с счётчиком в shared memory под семафором

// Прочитать значение счётчика с защитой семафором
inline int get_counter(SharedMemoryData* memory, SharedSemaphore& sem) {
    sem.wait();                     // Вход в критическую секцию
    int value = memory->counter;    // Читаем значение
    sem.signal();                   // Выход из критической секции
    return value;
}

// Установить значение счётчика
inline void set_counter(SharedMemoryData* memory, SharedSemaphore& sem, int value) {
    sem.wait();
    memory->counter = value;        // Записываем новое значение
    sem.signal();
}

// Увеличить счётчик на 1
inline void increment_counter(SharedMemoryData* memory, SharedSemaphore& sem) {
    sem.wait();
    ++memory->counter;              // Инкремент
    sem.signal();
}

// Обнулить всю структуру SharedMemoryData
inline void set_zero_shared_memory(SharedMemoryData* memory, SharedSemaphore& sem) {
    sem.wait();
    std::memset(memory, 0, sizeof(SharedMemoryData)); // Обнуляем всё, включая counter
    sem.signal();
}