#ifndef UTILS_H
#define UTILS_H

#include <string>

// Округляет float до 1 знака после запятой и возвращает строку вида "23.4"
std::string roundToTenth(float value);

// Вычисляет чексумму как сумму ASCII-кодов строки (без точки с запятой)
int calculateChecksum(const std::string& data);

// Проверяет формат и чексумму, возвращает true при успехе
bool validatePacket(const std::string& packet, float& outTemp);

#endif