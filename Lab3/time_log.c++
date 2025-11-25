#include <iostream>      // Для вывода ошибок (std::cerr)
#include <fstream>       // Для работы с файлом лога (std::ofstream)
#include <string>        // Для работы со std::string
#include <chrono>        // Для измерения времени (modern C++)
#include <iomanip>       // Для форматированного вывода числа (например, leading zeros)
#include <ctime>         // Для разбора времени на компоненты

#ifdef _WIN32
#include <windows.h>     // Для получения времени и PID в Windows
#else
#include <unistd.h>      // Для getpid() на POSIX
#include <sys/time.h>    // Для gettimeofday() на POSIX
#endif

// Кроссплатформенная функция формирования строки времени с точностью до миллисекунд
std::string time_to_str() {
    std::ostringstream oss; // Для удобного форматирования строки

#ifdef _WIN32
    SYSTEMTIME st;
    GetSystemTime(&st); // Получаем текущее время с миллисекундами

    // Форматируем дату и время с ведущими нулями
    oss << std::setfill('0')
        << std::setw(4) << st.wYear << '-'              // Год
        << std::setw(2) << st.wMonth << '-'             // Месяц
        << std::setw(2) << st.wDay << ' '               // День
        << std::setw(2) << st.wHour << ':'              // Часы
        << std::setw(2) << st.wMinute << ':'            // Минуты
        << std::setw(2) << st.wSecond << '.'            // Секунды
        << std::setw(3) << st.wMilliseconds;            // Миллисекунды
#else
    struct timeval tv;
    gettimeofday(&tv, nullptr); // Текущее время, включая микросекунды

    std::tm t;
    localtime_r(&tv.tv_sec, &t); // Перевод в локальное время

    // Форматирование аналогично Windows
    oss << std::setfill('0')
        << std::setw(4) << t.tm_year + 1900 << '-'      // Год
        << std::setw(2) << t.tm_mon + 1 << '-'          // Месяц
        << std::setw(2) << t.tm_mday << ' '             // День
        << std::setw(2) << t.tm_hour << ':'             // Часы
        << std::setw(2) << t.tm_min << ':'              // Минуты
        << std::setw(2) << t.tm_sec << '.'              // Секунды
        << std::setw(3) << (tv.tv_usec / 1000);         // Миллисекунды
#endif
    return oss.str();
}

// Логирование события в файл с проставлением времени и PID
void do_log(const std::string& data) {
    std::ofstream file("log.txt", std::ios::app); // Открываем лог-файл для дозаписи
    if (!file.is_open()) {
        std::cerr << "Failed to open log file" << std::endl; // Сообщение об ошибке, если не открылся
        return;
    }

#ifdef _WIN32
    int pid = GetCurrentProcessId(); // Получаем идентификатор процесса на Windows
#else
    int pid = getpid();              // Получаем идентификатор процесса на POSIX
#endif

    // Пишем в лог: [PID] время: текст лога
    file << "[" << pid << "] " << time_to_str() << ": " << data << std::endl;
    // std::ofstream сам закрывает файл и сбрасывает поток при завершении функции
}