cmake_minimum_required(VERSION 3.10.0)
project(OPENGLexperiment VERSION 1.0.0)
cmake_policy(SET CMP0072 NEW)

find_package(OpenGL REQUIRED)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

cmake_policy(SET CMP0072 NEW)

if(APPLE)
    # Force the linker and compiler to check the native Apple Silicon Homebrew directories
    include_directories(/opt/homebrew/include)
    link_directories(/opt/homebrew/lib)
endif()

add_executable(OPENGLexperiment 
src/triangle_mesh.cpp
src/main.cpp 
src/glad.c
)

target_include_directories(OPENGLexperiment PRIVATE dependencies/)

target_link_libraries(OPENGLexperiment glfw OpenGL::GL)

file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/src/shaders" 
     DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
