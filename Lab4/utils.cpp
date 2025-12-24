#include "utils.h"
#include <sstream>
#include <iomanip>
#include <cctype>
#include <stdexcept>

std::string roundToTenth(float value) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(1) << value;
    return ss.str();
}

int calculateChecksum(const std::string& data) {
    int sum = 0;
    for (char c : data) {
        sum += static_cast<unsigned char>(c);
    }
    return sum;
}

// Проверяет, что строка имеет формат: [-]ddd.d (ровно 1 цифра после точки)
bool isValidTempFormat(const std::string& s) {
    if (s.empty()) return false;

    size_t start = 0;
    if (s[0] == '-') {
        if (s.size() < 4) return false; // минимум "-0.0"
        start = 1;
    }

    // Находим точку
    size_t dotPos = s.find('.', start);
    if (dotPos == std::string::npos) return false;
    if (dotPos == start) return false; // нет цифр до точки (".5" — недопустимо)
    if (dotPos + 2 != s.size()) return false; // должно быть ровно 1 символ после точки

    // Проверяем: все до точки — цифры
    for (size_t i = start; i < dotPos; ++i) {
        if (!std::isdigit(static_cast<unsigned char>(s[i]))) {
            return false;
        }
    }

    // Проверяем: один символ после точки — цифра
    if (!std::isdigit(static_cast<unsigned char>(s[dotPos + 1]))) {
        return false;
    }

    return true;
}

bool validatePacket(const std::string& packet, float& outTemp) {
    size_t pos = packet.find(';');
    if (pos == std::string::npos || pos == 0 || pos == packet.size() - 1) {
        return false;
    }

    std::string tempStr = packet.substr(0, pos);
    std::string chkStr = packet.substr(pos + 1);

    // Проверка формата: только до десятых
    if (!isValidTempFormat(tempStr)) {
        return false;
    }

    // Преобразуем температуру
    try {
        outTemp = std::stof(tempStr);
    } catch (...) {
        return false;
    }

    // Проверяем чексумму
    int expected = calculateChecksum(tempStr);
    try {
        int received = std::stoi(chkStr);
        return (received == expected);
    } catch (...) {
        return false;
    }
}