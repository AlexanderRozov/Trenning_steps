cmake_minimum_required(VERSION 3.25)
project(tREND C)

set(CMAKE_C_STANDARD 11)

add_executable(tREND
        main.c
        src/db.c
        src/gui.c
        src/lang.c
        lib/sqlite3/sqlite3.c
)

target_include_directories(tREND PRIVATE
        src
        lib/sqlite3
        lib/nuklear
)

target_link_libraries(tREND PRIVATE
        gdi32
        user32
        msimg32
)
