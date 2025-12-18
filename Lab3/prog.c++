#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <cstring>
#endif

#include "time_log.hpp"
#include "sem_mng.hpp"
#include "mem_shr.hpp"
#include "thr.hpp"

// Имя семафора, который определяет "главный" процесс
#define SEM_NAME_MASTER "/semaphore_master"

// Глобальные указатели на разделяемую память и семафор
SharedMemory* g_shared_memory = nullptr;
SharedSemaphore* g_sem_master = nullptr;

// Путь к программе (для копий)
std::string g_program_path;

// Поток, увеличивающий счётчик каждые 300 мс
void* increment_thread(void* /*arg*/) {
    auto* data = g_shared_memory->get();
    while (true) {
        increment_counter(data, *g_sem_master);
#ifdef _WIN32
        Sleep(300);
#else
        usleep(300 * 1000);
#endif
    }
    return nullptr;
}

// Функция для записи времени и значения счётчика в лог
void log_counter() {
    auto* data = g_shared_memory->get();
    int counter_val = get_counter(data, *g_sem_master);
    char buf[64];
    std::snprintf(buf, sizeof(buf), "Counter value: %d", counter_val);
    do_log(buf);
}

// Поток, записывающий значение счётчика в лог каждую секунду
void* log_thread(void* /*arg*/) {
    while (true) {
        log_counter();
#ifdef _WIN32
        Sleep(1000);
#else
        sleep(1);
#endif
    }
    return nullptr;
}

// Поток, обрабатывающий пользовательский ввод
void* input_thread(void* /*arg*/) {
    auto* data = g_shared_memory->get();
    int new_value = 0;
    while (true) {
        std::printf("Enter a new counter value: ");
        if (std::scanf("%d", &new_value) == 1) {
            set_counter(data, *g_sem_master, new_value);
            std::printf("Counter updated to %d\n", new_value);
        } else {
            std::printf("Invalid input. Please enter an integer.\n");
            int c;
            while ((c = std::getchar()) != '\n' && c != EOF) {
                // чистим буфер
            }
        }
    }
    return nullptr;
}

// Обработка SIGINT 
void handle_signal(int /*sig*/) {
    std::printf("Shutdown...\n");
    try {
        if (g_sem_master) {
            // Освобождаем семафор, чтобы другие процессы могли стать "главными"
            // или корректно завершиться
            // В RAII-деструкторе всё и так закроется, но явный signal не помешает
        }
        // Освобождение ресурсов делегируем деструкторам в static/stack,
        // если объекты не в куче.
    } catch (...) {
        // Игнор ошибок при завершении
    }
    std::exit(0);
}

// Создание копии программы с параметром (--copy1 или --copy2)
int create_copy(const char* param) {
#ifdef _WIN32
    char program_path[MAX_PATH];
    if (GetModuleFileNameA(nullptr, program_path, MAX_PATH) == 0) {
        std::printf("Failed to get the program path\n");
        std::exit(1);
    }

    char command_line[MAX_PATH + 256];
    std::snprintf(command_line, sizeof(command_line), "\"%s\" %s", program_path, param);

    STARTUPINFOA si{};
    PROCESS_INFORMATION pi{};
    si.cb = sizeof(si);

    if (!CreateProcessA(
            nullptr,
            command_line,
            nullptr,
            nullptr,
            FALSE,
            0,
            nullptr,
            nullptr,
            &si,
            &pi)) {
        std::printf("Failed to create process\n");
        std::exit(1);
    }

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return static_cast<int>(pi.dwProcessId);

#else
    int pid = fork();
    if (pid == 0) {
        char* args[] = { const_cast<char*>(g_program_path.c_str()),
                         const_cast<char*>(param),
                         nullptr };
        execv(g_program_path.c_str(), args);
        std::perror("execv");
        std::exit(1);
    } else if (pid < 0) {
        std::printf("Failed to create child process\n");
        std::exit(1);
    }
    return pid;
#endif
}

// Поток, который раз в 3 секунды создаёт две копии программы
void* copies_thread(void* /*arg*/) {
    int copy_1_pid = 0;
    int copy_2_pid = 0;

    while (true) {
        if (copy_1_pid == 0) {
            copy_1_pid = create_copy("--copy1");
        } else {
            do_log("Copy 1 is still active. Skipping launch.");
        }

        if (copy_2_pid == 0) {
            copy_2_pid = create_copy("--copy2");
        } else {
            do_log("Copy 2 is still active. Skipping launch.");
        }

#ifdef _WIN32
        Sleep(3000);
#else
        usleep(3000 * 1000);
#endif

        if (copy_1_pid != 0 && check_process_finished(copy_1_pid) == 1) {
            copy_1_pid = 0;
        }
        if (copy_2_pid != 0 && check_process_finished(copy_2_pid) == 1) {
            copy_2_pid = 0;
        }
    }
    return nullptr;
}

// Логика копии 1
void handle_copy_1() {
    auto* data = g_shared_memory->get();
    do_log("(Copy1) Started");
    increment_counter(data, *g_sem_master); // +1
    
    for (int i = 0; i < 9; ++i) {
        increment_counter(data, *g_sem_master);
    }
    do_log("(Copy1) Exit");
    std::exit(0);
}

// Логика копии 2
void handle_copy_2() {
    auto* data = g_shared_memory->get();
    do_log("(Copy2) Started");

    // *2
    g_sem_master->wait();
    data->counter *= 2;
    g_sem_master->signal();

#ifdef _WIN32
    Sleep(2000);
#else
    usleep(2000 * 1000);
#endif

    // /2
    g_sem_master->wait();
    data->counter /= 2;
    g_sem_master->signal();

    do_log("(Copy2) Exit");
    std::exit(0);
}

int main(int argc, char* argv[]) {
    try {
        // Инициализация разделяемой памяти и семафора для счётчика
        SharedMemory shm("/my_shared_memory");
        SharedSemaphore sem_counter("/shared_memory_semaphore");
        g_shared_memory = &shm;
        g_sem_master = &sem_counter;

        auto* data = g_shared_memory->get();

        // Если процесс запущен как копия
        if (argc > 1) {
            if (strcmp(argv[1], "--copy1") == 0) {
                handle_copy_1();
            } else if (strcmp(argv[1], "--copy2") == 0) {
                handle_copy_2();
            }
        }

        g_program_path = argv[0];

        thread_t inc_thread{};
        thread_t logger_thread{};
        thread_t inp_thread{};
        thread_t cop_thread{};

        if (thread_create(&inc_thread, increment_thread, nullptr) != 0) {
            std::perror("increment_thread");
            return 1;
        }

        if (thread_create(&inp_thread, input_thread, nullptr) != 0) {
            std::perror("input_thread");
            return 1;
        }

        // Семафор "главного" процесса
        SharedSemaphore sem_master(SEM_NAME_MASTER);
        do_log("Started");

        // Пытаемся стать главным процессом
        sem_master.wait();
        do_log("I'm master process");
        std::signal(SIGINT, handle_signal);

        // Обнуляем разделяемую память один раз в главном процессе
        set_zero_shared_memory(data, sem_counter);

        if (thread_create(&logger_thread, log_thread, nullptr) != 0) {
            std::perror("log_thread");
            return 1;
        }

        if (thread_create(&cop_thread, copies_thread, nullptr) != 0) {
            std::perror("copies_thread");
            return 1;
        }

        // Ждём потоки
        thread_join(inc_thread);
        thread_join(inp_thread);
        thread_join(logger_thread);
        thread_join(cop_thread);
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "Error: %s\n", ex.what());
        return 1;
    }

    return 0;
}