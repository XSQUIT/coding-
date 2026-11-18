cmake_minimum_required(VERSION 3.12)
project(PhysicsSandbox C)

set(CMAKE_C_STANDARD 99)

add_executable(sandbox 
    src/main.c 
    src/physics.c
    src/rendering.c
)

target_include_directories(sandbox PRIVATE 
    include
    /opt/homebrew/include
)

target_link_directories(sandbox PRIVATE 
    /opt/homebrew/lib
)

target_link_libraries(sandbox 
    raylib
    "-framework OpenGL"
    "-framework Cocoa"
    "-framework IOKit"
    "-framework CoreVideo"
)
