#ifndef LIBRARY_HPP
#define LIBRARY_HPP

#include <string>

std::pair<int, int> start_process(const std::string& command, bool wait = false);

#endif // LIBRARY_HPP