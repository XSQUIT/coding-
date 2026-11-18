cmake_minimum_required(VERSION 3.12)
project(PhysicsSandbox C CXX)

set(CMAKE_CXX_STANDARD 26)

add_executable(sandbox 
    src/main.cpp 
    src/physics.cpp
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
