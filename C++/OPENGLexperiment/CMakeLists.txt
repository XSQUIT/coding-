cmake_minimum_required(VERSION 3.10.0)
project(OPENGLexperiment VERSION 1.0.0)
cmake_policy(SET CMP0072 NEW)

find_package(OpenGL REQUIRED)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

cmake_policy(SET CMP0072 NEW)

add_executable(OPENGLexperiment 
src/main.cpp 
src/glad.c
)

target_include_directories(OPENGLexperiment PRIVATE dependencies/)

target_link_libraries(OPENGLexperiment glfw OpenGL::GL)

