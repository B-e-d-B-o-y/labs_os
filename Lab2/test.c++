#include "library.h++"
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#endif

int main() {

    // Чиним кодировку консольного вывода для Windows и указываем подпрограмму для запуска (sleep/timeout)
    #ifdef _WIN32
    std::string cmd_name = "cmd /c timeout /t 5 >nul 2>&1";
    SetConsoleOutputCP(CP_UTF8);
    #else
    std::string cmd_name = "sleep 5";
    #endif

    // Пример: запуск без ожидания, запускается sleep на 5 секунд
    std::cout << "Процесс без ожидания..." << std::endl;
    auto result1 = start_process(cmd_name, false);
    if (result1.first == 0) {
        std::cout << "Процесс запущен в фоновом режиме." << std::endl;
    } else {
        std::cout << "Ошибка запуска процесса." << std::endl;
    }

    // Пример: ожидание завершения, запускается sleep на 5 секунд
    std::cout << "Процесс с ожиданием..." << std::endl;
    auto result2 = start_process(cmd_name, true);
    if (result2.first == 0) {
        std::cout << "Процесс с ожиданием завершился с кодом: " << result2.second << std::endl;
    } else {
        std::cout << "Ошибка запуска процесса." << std::endl;
    }
    return 0;
}
