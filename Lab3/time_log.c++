#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <iomanip>
#include <ctime>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/time.h>
#endif
#include "time_log.hpp"

std::string time_to_str() {
    std::ostringstream oss;
#ifdef _WIN32
    SYSTEMTIME st;
    GetLocalTime(&st);  // Локальное время
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
    gettimeofday(&tv, nullptr);
    std::tm t;
    localtime_r(&tv.tv_sec, &t);
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

void do_log(const std::string& data) {
#ifdef _WIN32
    int pid = static_cast<int>(GetCurrentProcessId());
#else
    int pid = static_cast<int>(getpid());
#endif
    
    // Кроссплатформенная блокировка файла
    std::ofstream file("lab3.log", std::ios::app);
    if (!file.is_open()) {
        std::cerr << "Failed to open log file" << std::endl;
        return;
    }
    
    file << "[" << time_to_str() << "] PID=" << pid << ": " << data << std::endl;
    file.flush();
}