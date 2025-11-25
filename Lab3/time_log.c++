#include <iostream>      // std::cerr
#include <fstream>       // std::ofstream
#include <string>        // std::string
#include <sstream>       // std::ostringstream
#include <chrono>
#include <iomanip>       // std::setfill, std::setw
#include <ctime>         // std::tm

#ifdef _WIN32
#include <windows.h>     // SYSTEMTIME, GetSystemTime, GetCurrentProcessId
#else
#include <unistd.h>      // getpid
#include <sys/time.h>    // gettimeofday
#endif

#include "time_log.hpp"  // объявления time_to_str и do_log

// Кроссплатформенная функция формирования строки времени с точностью до миллисекунд
std::string time_to_str() {
    std::ostringstream oss; // Для удобного форматирования строки

#ifdef _WIN32
    SYSTEMTIME st;
    GetSystemTime(&st); // Получаем текущее время с миллисекундами

    // Форматируем дату и время с ведущими нулями
    oss << std::setfill('0')
        << std::setw(4) << st.wYear << '-'
        << std::setw(2) << st.wMonth << '-'
        << std::setw(2) << st.wDay << ' '
        << std::setw(2) << st.wHour << ':'
        << std::setw(2) << st.wMinute << ':'
        << std::setw(2) << st.wSecond << '.'
        << std::setw(3) << st.wMilliseconds;
#else
    struct timeval tv;
    gettimeofday(&tv, nullptr); // Текущее время, включая микросекунды

    std::tm t;
    localtime_r(&tv.tv_sec, &t); // Перевод в локальное время

    // Форматирование аналогично Windows
    oss << std::setfill('0')
        << std::setw(4) << t.tm_year + 1900 << '-'
        << std::setw(2) << t.tm_mon + 1 << '-'
        << std::setw(2) << t.tm_mday << ' '
        << std::setw(2) << t.tm_hour << ':'
        << std::setw(2) << t.tm_min << ':'
        << std::setw(2) << t.tm_sec << '.'
        << std::setw(3) << (tv.tv_usec / 1000);
#endif
    return oss.str();
}

// Логирование события в файл с проставлением времени и PID
void do_log(const std::string& data) {
    std::ofstream file("log.txt", std::ios::app); // Открываем лог-файл для дозаписи
    if (!file.is_open()) {
        std::cerr << "Failed to open log file" << std::endl;
        return;
    }

#ifdef _WIN32
    int pid = static_cast<int>(GetCurrentProcessId());
#else
    int pid = static_cast<int>(getpid());
#endif

    // Пишем в лог: [PID] время: текст лога
    file << "[" << pid << "] " << time_to_str() << ": " << data << std::endl;
}
