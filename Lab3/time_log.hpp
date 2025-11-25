#pragma once

#include <string>

// Возвращает строку с текущим временем (дата + время + миллисекунды)
std::string time_to_str();

// Пишет в log.txt строку с PID, временем и сообщением data
void do_log(const std::string& data);