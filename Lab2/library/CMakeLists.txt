set(LIBRARY_NAME library)

set(SRC_FILES
    library.c++ # основной исходник для реализации на C++
)

add_library(${LIBRARY_NAME} STATIC ${SRC_FILES})

target_include_directories(${LIBRARY_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(${LIBRARY_NAME} PROPERTIES
    PUBLIC_HEADER library.hpp # здесь указывайте нужный публичный хедер для пользователей
)

install(TARGETS ${LIBRARY_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    PUBLIC_HEADER DESTINATION include
)

install(FILES library.hpp DESTINATION include)
