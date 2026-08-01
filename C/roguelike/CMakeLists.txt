cmake_minimum_required(VERSION 3.10)
project(descent C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_C_FLAGS_RELEASE "-O2")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")

find_package(Curses REQUIRED)

include_directories(${CURSES_INCLUDE_DIR} include)

add_executable(descent
    src/main.c
    src/map.c
    src/entity.c
    src/combat.c
    src/item.c
    src/game_state.c
    src/render.c
)

target_link_libraries(descent ${CURSES_LIBRARIES} m)

# Headless logic test suite -- exercises map generation, combat, items,
# and monster AI without touching ncurses, so it can run anywhere
# (including CI or a terminal-less sandbox).
add_executable(test_headless
    src/test_headless.c
    src/map.c
    src/entity.c
    src/combat.c
    src/item.c
    src/game_state.c
)
target_link_libraries(test_headless m)
