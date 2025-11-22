// library.cpp
#include "library.h++"
#include <cstdlib>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#endif

std::pair<int, int> start_process(const std::string& command, bool wait) {
    if (command.empty()) return {-1, 0};

#ifdef _WIN32
    STARTUPINFOA si = {0};
    PROCESS_INFORMATION pi = {0};
    si.cb = sizeof(si);
    std::string cmd = command;

    if (!CreateProcessA(
            NULL, cmd.data(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        return {-1, 0};
    }

    CloseHandle(pi.hThread);

    int exit_code = 0;
    if (wait) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        DWORD status = 0;
        if (!GetExitCodeProcess(pi.hProcess, &status)) {
            CloseHandle(pi.hProcess);
            return {-1, 0};
        }
        exit_code = static_cast<int>(status);
        CloseHandle(pi.hProcess);
    }
    return {0, exit_code};

#else
    pid_t pid = fork();
    if (pid < 0) {
        return {-1, 0};
    } else if (pid == 0) {
        execl("/bin/sh", "sh", "-c", command.c_str(), (char *)NULL);
        std::exit(errno);
    } else {
        int exit_code = 0;
        if (wait) {
            int status = 0;
            if (waitpid(pid, &status, 0) == -1) {
                return {-1, 0};
            }
            exit_code = (WIFEXITED(status)) ? WEXITSTATUS(status) : -1;
        }
        return {0, exit_code};
    }
#endif
}
