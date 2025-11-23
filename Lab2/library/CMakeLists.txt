# Устанавливается имя библиотеки для использования в дальнейшем
set(LIBRARY_NAME library)
# Список исходных файлов для библиотеки
set(SRC_FILES library.c++)
# Создаётся статическая библиотека с заданным именем и исходниками
add_library(${LIBRARY_NAME} STATIC ${SRC_FILES})
# Добавляется директория (текущая папка) к путям поиска заголовочных файлов для библиотеки.
# PUBLIC означает, что этот путь будет виден всем, кто подключает эту библиотеку.
target_include_directories(${LIBRARY_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# Указываем публичный хедер
set_target_properties(${LIBRARY_NAME} PROPERTIES PUBLIC_HEADER library.h++)