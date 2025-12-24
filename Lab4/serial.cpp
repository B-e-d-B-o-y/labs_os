#include "serial.h"
#include <stdexcept>
#include <chrono>
#include <thread>
#include <cstring>
#include <string>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <fcntl.h>
    #include <termios.h>
    #include <unistd.h>
    #include <poll.h>
#endif

SerialPort::SerialPort(const std::string& portName) {
#ifdef _WIN32
    std::string fullPort = portName;
    if (portName.compare(0, 3, "COM") == 0 && portName.size() > 4) {
        fullPort = "\\\\.\\" + portName;
    }
    handle = CreateFileA(
        fullPort.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0, nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (handle == INVALID_HANDLE_VALUE) {
        throw std::runtime_error("Failed to open port: " + portName);
    }

    DCB dcb = {0};
    dcb.DCBlength = sizeof(dcb);
    dcb.BaudRate = CBR_9600;
    dcb.ByteSize = 8;
    dcb.StopBits = ONESTOPBIT;
    dcb.Parity = NOPARITY;
    if (!SetCommState(static_cast<HANDLE>(handle), &dcb)) {
        CloseHandle(static_cast<HANDLE>(handle));
        throw std::runtime_error("Failed to configure port: " + portName);
    }

    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 1000;  // 1 секунда
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = 100;
    timeouts.WriteTotalTimeoutMultiplier = 0;
    SetCommTimeouts(static_cast<HANDLE>(handle), &timeouts);

#else
    fd = open(portName.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        throw std::runtime_error("Failed to open port: " + portName);
    }

    termios tty;
    memset(&tty, 0, sizeof tty);
    if (tcgetattr(fd, &tty) != 0) {
        close(fd);
        throw std::runtime_error("Failed to get port attributes");
    }

    cfmakeraw(&tty);
    tty.c_cflag &= ~CSTOPB;   // 1 stop bit
    tty.c_cflag &= ~PARENB;   // No parity
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;       // 8 bits
    tty.c_cflag &= ~CRTSCTS;  // No flow control

    // Скорость 9600
    cfsetospeed(&tty, B9600);
    cfsetispeed(&tty, B9600);

    tty.c_cc[VMIN] = 0;       // Мин. количество символов для чтения
    tty.c_cc[VTIME] = timeoutMs / 100; // Таймаут в десятках секунд

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        close(fd);
        throw std::runtime_error("Failed to set port attributes");
    }
#endif
}

SerialPort::~SerialPort() {
#ifdef _WIN32
    if (handle != nullptr) {
        CloseHandle(static_cast<HANDLE>(handle));
    }
#else
    if (fd >= 0) {
        close(fd);
    }
#endif
}

bool SerialPort::isOpen() const {
#ifdef _WIN32
    return handle != nullptr;
#else
    return fd >= 0;
#endif
}

void SerialPort::writeLine(std::string_view data) {
    std::string msg = std::string(data) + "\n";

#ifdef _WIN32
    DWORD bytesWritten = 0;
    WriteFile(static_cast<HANDLE>(handle), msg.c_str(), static_cast<DWORD>(msg.size()), &bytesWritten, nullptr);
#else
    write(fd, msg.c_str(), msg.size());
#endif
}

std::string SerialPort::readLine(int timeoutMs) {
    std::string line;
    const int maxWaitMs = timeoutMs;
    const auto start = std::chrono::steady_clock::now();

#ifdef _WIN32
    while (true) {
        char c;
        DWORD bytesRead = 0;
        bool success = ReadFile(static_cast<HANDLE>(handle), &c, 1, &bytesRead, nullptr);
        if (success && bytesRead == 1) {
            if (c == '\n') break;
            if (c != '\r') line += c;
        } else {
            // Проверяем таймаут
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
            if (elapsed >= maxWaitMs) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
#else
    while (true) {
        struct pollfd pfd = { fd, POLLIN, 0 };
        int ret = poll(&pfd, 1, 1); // ждём 1 мс
        if (ret > 0 && (pfd.revents & POLLIN)) {
            char c;
            ssize_t n = read(fd, &c, 1);
            if (n == 1) {
                if (c == '\n') break;
                if (c != '\r') line += c;
            }
        }

        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
        if (elapsed >= maxWaitMs) break;
    }
#endif

    return line;
}