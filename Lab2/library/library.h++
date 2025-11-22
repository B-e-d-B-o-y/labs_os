#ifndef LIBRARY_HPP
#define LIBRARY_HPP

#include <string>

/**
 * @brief Запускает указанную программу в фоновом или синхронном режиме.
 * @param command Команда для выполнения (путь к исполняемому файлу или shell-команда).
 * @param wait Если true — ждать завершения процесса и возвращать код завершения; если false — не ждать.
 * @return std::pair<int, int> — первый элемент: код ошибки (0 — успех, -1 — ошибка), второй: exit-код дочернего процесса (если wait == true, иначе 0).
 *
 * @note Если wait == false, exit-код не определён (0).
 */
std::pair<int, int> start_process(const std::string& command, bool wait = false);

#endif // LIBRARY_HPP