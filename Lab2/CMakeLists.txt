# Требует не менее определенной версии CMake. Защищает от ошибок при использовании новых функций. Обеспечивает совместимость и правильное поведение сборки.
cmake_minimum_required(VERSION 3.16)
# Объявление проект с именем "Library" на языке C++
project(Library LANGUAGES CXX)
# Устанавливается стандарт C++17, чтбоы не было ошибок при сборке
set(CMAKE_CXX_STANDARD 17)
# Использование именно выбранного стандарта C++ — обязательное требование для компилятора
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Подключается подпроект (каталог library), где лежит исходный код и отдельный CMakeLists.txt библиотеки
add_subdirectory(library)
# Определяется имя исполняемого файла (тестовая программа)
set(EXAMPLE_NAME test)
# Создаётся исполняемый файл из исходника test.c++
add_executable(${EXAMPLE_NAME} test.c++)
# Линкуется исполняемый файл с библиотекой, собранной в подпроекте "library"
target_link_libraries(${EXAMPLE_NAME} PRIVATE library)