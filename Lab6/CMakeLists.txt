cmake_minimum_required(VERSION 3.16)
project(Lab6_GUI LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Для Windows с MinGW
set(CMAKE_PREFIX_PATH "C:/Qt/6.7.0/mingw_64")

# Находим Qt6 с модулем Charts
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network Charts)

# Создаем исполняемый файл
add_executable(temperature_gui
    main.cpp
    mainwindow.cpp
    httpclient.cpp
    chartwidget_qtcharts.cpp  # Новый файл с Qt Charts
)

# Включаем директории
target_include_directories(temperature_gui PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Подключаем библиотеки
target_link_libraries(temperature_gui
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::Charts
)

# Копируем DLL
if(WIN32)
    add_custom_command(TARGET temperature_gui POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "C:/Qt/6.7.0/mingw_64/bin/Qt6Core.dll"
        $<TARGET_FILE_DIR:temperature_gui>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "C:/Qt/6.7.0/mingw_64/bin/Qt6Widgets.dll"
        $<TARGET_FILE_DIR:temperature_gui>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "C:/Qt/6.7.0/mingw_64/bin/Qt6Network.dll"
        $<TARGET_FILE_DIR:temperature_gui>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "C:/Qt/6.7.0/mingw_64/bin/Qt6Charts.dll"
        $<TARGET_FILE_DIR:temperature_gui>
    )
endif()