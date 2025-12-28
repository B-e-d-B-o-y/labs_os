#include "serial.h"
#include "utils.h"
#include <iostream>
#include <random>
#include <thread>
#include <chrono>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif


int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    const std::string PORT_NAME =
#ifdef _WIN32
        "COM6";  // эмулятор пишет в COM6
#else
        "/dev/pts2"; 
#endif

    try {
        SerialPort port(PORT_NAME);
        std::cout << "Emulator started. Sending temperature to " << PORT_NAME << " every 60 seconds.\n";

        // Генератор случайных чисел
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dis(15.0f, 35.0f);

        while (true) {
            float rawTemp = dis(gen);
            std::string tempStr = roundToTenth(rawTemp); // например: "23.4"
            int chk = calculateChecksum(tempStr);
            std::string packet = tempStr + ";" + std::to_string(chk);

            port.writeLine(packet);
            std::cout << "Sent: " << packet << "\n";

            // Ждём 60 секунд
            std::this_thread::sleep_for(std::chrono::seconds(60));
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}