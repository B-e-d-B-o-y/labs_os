#ifndef SERIAL_H
#define SERIAL_H

#include <string>
#include <string_view>

class SerialPort {
public:
    explicit SerialPort(const std::string& portName);
    ~SerialPort();

    SerialPort(const SerialPort&) = delete;
    SerialPort& operator=(const SerialPort&) = delete;

    bool isOpen() const;
    std::string readLine(int timeoutMs = 1000);  // читает до '\n'
    void writeLine(std::string_view data);

private:
#ifdef _WIN32
    void* handle = nullptr;  // HANDLE -> void* чтобы не тянуть windows.h в заголовок
#else
    int fd = -1;
#endif
};

#endif // SERIAL_H