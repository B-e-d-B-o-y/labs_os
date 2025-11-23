#include "library.h++"          // Подключаем заголовок библиотеки с функцией запуска внешнего процесса
#include <iostream>             // Для вывода сообщений в консоль
#include <string>               // Для работы со строками

#ifdef _WIN32
#include <windows.h>            // Для установки кодировки консоли на Windows
#endif

// Функция для демонстрации запуска процесса, принимает команду и флаг ожидания
void run_test(const std::string& cmd_name, bool wait) {
    // Выводим поясняющее сообщение о режиме запуска
    std::cout << (wait ? "Запуск с ожиданием..." : "Запуск без ожидания...") << std::endl;

    // Вызываем функцию из библиотеки, запускаем процесс
    auto result = start_process(cmd_name, wait);

    // Проверяем результат (0 — успех)
    if (result.first == 0) {
        // Для режима ожидания выводим exit-код дочернего процесса
        if (wait) {
            std::cout << "Процесс завершён, exit-код: " << result.second << std::endl;
        } else {
            // Если не ждали — просто информируем о запуске
            std::cout << "Процесс запущен в фоновом режиме." << std::endl;
        }
    } else {
        // В случае ошибки выводим код ошибки и команду
        std::cout << "Ошибка запуска процесса (код: " << result.first << "). Команда: " << cmd_name << std::endl;
    }
}

// Основная функция программы
int main() {
#ifdef _WIN32
    // Команда для "засыпания" на Windows (через timeout)
    std::string cmd_name = "cmd /c timeout /t 5 >nul 2>&1";
    SetConsoleOutputCP(CP_UTF8); // Исправляем кодировку консоли для корректного вывода текста
#else
    // Команда для "засыпания" на POSIX-системах (Linux, MacOS)
    std::string cmd_name = "sleep 5";
#endif

    // Запускаем процесс без ожидания завершения
    run_test(cmd_name, false);
    // Запускаем процесс с ожиданием завершения
    run_test(cmd_name, true);

    return 0; // Завершаем программу с кодом 0 (успех)
}
