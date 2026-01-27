#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <cstring>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#endif
#include "time_log.hpp"
#include "sem_mng.hpp"
#include "mem_shr.hpp"
#include "thr.hpp"
#include <process.h>

#define SEM_NAME_MASTER "/lab3_master_sem"
#define LOG_FILE "lab3.log"

SharedMemory* g_shared_memory = nullptr;
SharedSemaphore* g_sem_counter = nullptr;
SharedSemaphore* g_sem_master = nullptr;
std::string g_program_path;
bool g_is_master = false;  // ← Флаг главного процесса

void* increment_thread(void* /*arg*/) {
    while (true) {
        increment_counter(g_shared_memory->get(), *g_sem_counter);
#ifdef _WIN32
        Sleep(300);
#else
        usleep(300 * 1000);
#endif
    }
    return nullptr;
}

void log_counter() {
    int counter_val = get_counter(g_shared_memory->get(), *g_sem_counter);
    char buf[128];
    std::snprintf(buf, sizeof(buf), "TIME=%s PID=%d COUNTER=%d",
        time_to_str().c_str(), getpid(), counter_val);
    do_log(buf);
}

void* log_thread(void* /*arg*/) {
    while (true) {
        if (g_is_master) {
            log_counter();
        }
#ifdef _WIN32
        Sleep(1000);
#else
        sleep(1);
#endif
    }
    return nullptr;
}

void* input_thread(void* /*arg*/) {
    int new_value = 0;
    while (true) {
        std::printf("\n[PID %d] Enter new counter value (or 'q' to quit): ", getpid());
        char input[32];
        if (std::scanf("%31s", input) != 1) continue;
        
        if (input[0] == 'q' || input[0] == 'Q') {
            std::printf("Exiting process PID %d...\n", getpid());
            std::exit(0);
        }
        
        try {
            new_value = std::stoi(input);
            set_counter(g_shared_memory->get(), *g_sem_counter, new_value);
            std::printf("[PID %d] Counter set to %d\n", getpid(), new_value);
        } catch (...) {
            std::printf("[PID %d] Invalid input. Enter an integer.\n", getpid());
            while (std::getchar() != '\n' && !std::feof(stdin));
        }
    }
    return nullptr;
}

void handle_signal(int /*sig*/) {
    std::printf("\n[PID %d] Shutting down...\n", getpid());
    std::exit(0);
}

int create_copy(const char* param) {
#ifdef _WIN32
    char program_path[MAX_PATH];
    if (GetModuleFileNameA(nullptr, program_path, MAX_PATH) == 0) {
        std::printf("Failed to get program path\n");
        return -1;
    }
    char command_line[MAX_PATH + 256];
    std::snprintf(command_line, sizeof(command_line), "\"%s\" %s", program_path, param);
    
    STARTUPINFOA si{};
    PROCESS_INFORMATION pi{};
    si.cb = sizeof(si);
    if (!CreateProcessA(nullptr, command_line, nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi)) {
        std::printf("CreateProcess failed, error: %lu\n", GetLastError());
        return -1;
    }
    CloseHandle(pi.hThread);
    return static_cast<int>(pi.dwProcessId);
#else
    pid_t pid = fork();
    if (pid == 0) {
        char* args[] = { const_cast<char*>(g_program_path.c_str()), const_cast<char*>(param), nullptr };
        execv(g_program_path.c_str(), args);
        std::perror("execv failed");
        std::exit(1);
    } else if (pid < 0) {
        std::perror("fork failed");
        return -1;
    }
    return static_cast<int>(pid);
#endif
}

void* copies_thread(void* /*arg*/) {
    int copy1_pid = -1;
    int copy2_pid = -1;
    
    while (true) {
        // Ждём 3 секунды ДО проверки и запуска копий (требование 5)
        if (g_is_master) {
#ifdef _WIN32
            Sleep(3000);
#else
            sleep(3);
#endif
            
            // Проверяем завершение предыдущих копий (требование 5c)
            if (copy1_pid != -1) {
                int status = check_process_finished(copy1_pid);
                if (status == 1) {
                    copy1_pid = -1;  // Копия завершилась
                } else if (status == 0) {
                    do_log("WARNING: Copy 1 still running, skipping new launch");
                    // Не запускаем новую копию
                    continue;
                }
            }
            
            if (copy2_pid != -1) {
                int status = check_process_finished(copy2_pid);
                if (status == 1) {
                    copy2_pid = -1;  // Копия завершилась
                } else if (status == 0) {
                    do_log("WARNING: Copy 2 still running, skipping new launch");
                    // Не запускаем новую копию
                    continue;
                }
            }
            
            // Запускаем копии ТОЛЬКО если предыдущие завершились
            if (copy1_pid == -1) {
                copy1_pid = create_copy("--copy1");
                if (copy1_pid != -1) {
                    do_log("Launched copy 1 (PID=" + std::to_string(copy1_pid) + ")");
                }
            }
            
            if (copy2_pid == -1) {
                copy2_pid = create_copy("--copy2");
                if (copy2_pid != -1) {
                    do_log("Launched copy 2 (PID=" + std::to_string(copy2_pid) + ")");
                }
            }
        } else {
            // Не-главный процесс ждёт 3 секунды и ничего не делает
#ifdef _WIN32
            Sleep(3000);
#else
            sleep(3);
#endif
        }
    }
    return nullptr;
}

void handle_copy_1() {
    do_log("COPY1_STARTED PID=" + std::to_string(getpid()) + " TIME=" + time_to_str());
    
    // Увеличиваем счётчик на 10 (требование 5a)
    g_sem_counter->wait();
    g_shared_memory->get()->counter += 10;
    g_sem_counter->signal();
    
    do_log("COPY1_EXIT PID=" + std::to_string(getpid()) + " TIME=" + time_to_str());
    std::exit(0);
}

void handle_copy_2() {
    do_log("COPY2_STARTED PID=" + std::to_string(getpid()) + " TIME=" + time_to_str());
    
    // Увеличиваем счётчик в 2 раза (требование 5b)
    g_sem_counter->wait();
    g_shared_memory->get()->counter *= 2;
    g_sem_counter->signal();
    
    // Ждём 2 секунды
#ifdef _WIN32
    Sleep(2000);
#else
    sleep(2);
#endif
    
    // Уменьшаем счётчик в 2 раза
    g_sem_counter->wait();
    g_shared_memory->get()->counter /= 2;
    g_sem_counter->signal();
    
    do_log("COPY2_EXIT PID=" + std::to_string(getpid()) + " TIME=" + time_to_str());
    std::exit(0);
}

int main(int argc, char* argv[]) {
    try {
        // Инициализация разделяемых ресурсов
        SharedMemory shm("/lab3_shared_memory");
        SharedSemaphore sem_counter("/lab3_counter_sem");
        SharedSemaphore sem_master(SEM_NAME_MASTER);
        
        g_shared_memory = &shm;
        g_sem_counter = &sem_counter;
        g_sem_master = &sem_master;
        g_program_path = argv[0];
        
        // Если запущен как копия
        if (argc > 1) {
            if (strcmp(argv[1], "--copy1") == 0) {
                handle_copy_1();
            } else if (strcmp(argv[1], "--copy2") == 0) {
                handle_copy_2();
            }
            return 0;
        }
        
        // === ОПРЕДЕЛЕНИЕ ГЛАВНОГО ПРОЦЕССА ===
        // Пытаемся захватить семафор БЕЗ БЛОКИРОВКИ
        // Первый процесс, захвативший семафор, становится главным
        if (g_sem_master->try_wait()) {
            g_is_master = true;
            // Обнуляем счётчик ТОЛЬКО в главном процессе
            set_zero_shared_memory(g_shared_memory->get(), *g_sem_counter);
            // Освобождаем семафор — флаг главенства сохранён в переменной g_is_master
            g_sem_master->signal();
        } else {
            g_is_master = false;
        }
        
        // === ВЫВОД СТАТУСА В КОНСОЛЬ ===
        std::printf("\n");
        std::printf("========================================\n");
        std::printf(" Process ID: %d\n", getpid());
        if (g_is_master) {
            std::printf(" Status:     [MAIN PROCESS] ✅\n");
            std::printf(" Role:       Logs counter + launches copies\n");
        } else {
            std::printf(" Status:     [AUXILIARY PROCESS] ⚙️\n");
            std::printf(" Role:       Modifies counter only\n");
        }
        std::printf("========================================\n");
        std::printf("\n");
        
        // Запись в лог при старте с PID и временем (требование 1)
        char start_msg[128];
        std::snprintf(start_msg, sizeof(128), "PROCESS_STARTED PID=%d TIME=%s IS_MASTER=%s",
            getpid(), time_to_str().c_str(), g_is_master ? "YES" : "NO");
        do_log(start_msg);
        
        // Установка обработчика сигналов
        std::signal(SIGINT, handle_signal);
#ifndef _WIN32
        std::signal(SIGTERM, handle_signal);
#endif
        
        // Запуск потоков
        thread_t inc_thread{}, logger_thread{}, inp_thread{}, cop_thread{};
        
        if (thread_create(&inc_thread, increment_thread, nullptr) != 0) {
            std::perror("Failed to create increment thread");
            return 1;
        }
        if (thread_create(&logger_thread, log_thread, nullptr) != 0) {
            std::perror("Failed to create logger thread");
            return 1;
        }
        if (thread_create(&inp_thread, input_thread, nullptr) != 0) {
            std::perror("Failed to create input thread");
            return 1;
        }
        if (thread_create(&cop_thread, copies_thread, nullptr) != 0) {
            std::perror("Failed to create copies thread");
            return 1;
        }
        
        // Основной цикл — ожидание ввода пользователя
        while (true) {
#ifdef _WIN32
            Sleep(100);
#else
            usleep(100000);
#endif
        }
        
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "Error: %s\n", ex.what());
        return 1;
    }
    return 0;
}