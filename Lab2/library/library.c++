// Заголовок с объявлением интерфейса функции
#include "library.h++"
#include <string>
#include <utility>

#ifdef _WIN32
#include <windows.h> // Работа с процессами в Windows
#else
#include <unistd.h>      // fork, execl
#include <sys/wait.h>    // waitpid, WIFEXITED
#include <errno.h>       // код ошибки
#endif

// Запускает внешний процесс по строке-команде.
// Если wait == true, ожидает завершения дочернего процесса и возвращает его код возврата.
// Возвращает: {код ошибки, exit_code дочернего процесса (если wait)}
std::pair<int, int> start_process(const std::string& command, bool wait) {
    // Проверка на пустую команду
    if (command.empty()) return {-1, 0};

#ifdef _WIN32
    // Подготовка структур Windows для создания процесса
    STARTUPINFOA si = {0};
    PROCESS_INFORMATION pi = {0};
    si.cb = sizeof(si);

    // Копируем команду в изменяемую строку
    std::string cmd = command;

    // Запускаем процесс
    if (!CreateProcessA(
            NULL, cmd.data(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        // Если ошибка — возвращаем -1
        return {-1, 0};
    }

    // Закрываем дескриптор потока (процесс остаётся)
    CloseHandle(pi.hThread);

    int exit_code = 0;
    if (wait) {
        // Ожидаем завершения процесса
        WaitForSingleObject(pi.hProcess, INFINITE);
        DWORD status = 0;
        // Получаем exit code
        if (!GetExitCodeProcess(pi.hProcess, &status)) {
            CloseHandle(pi.hProcess);
            return {-1, 0};
        }
        exit_code = static_cast<int>(status);
        // Закрываем дескриптор процесса
        CloseHandle(pi.hProcess);
    }
    // Возвращаем {0, exit_code} — если всё успешно
    return {0, exit_code};

#else
    // Создаём дочерний процесс
    pid_t pid = fork();
    if (pid < 0) {
        // Ошибка fork
        return {-1, 0};
    } else if (pid == 0) {
        // Дочерний процесс: запускаем команду через shell
        execl("/bin/sh", "sh", "-c", command.c_str(), (char *)NULL);
        // Если execl не выполнен — завершаем процесс и возвращаем код ошибки
        std::exit(errno);
    } else {
        int exit_code = 0;
        if (wait) {
            int status = 0;
            // Ожидаем завершения дочернего процесса
            if (waitpid(pid, &status, 0) == -1) {
                return {-1, 0};
            }
            // Получаем exit code дочернего процесса
            exit_code = (WIFEXITED(status)) ? WEXITSTATUS(status) : -1;
        }
        // Возвращаем {0, exit_code}
        return {0, exit_code};
    }
#endif
}
