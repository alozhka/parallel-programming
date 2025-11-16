cmake_minimum_required(VERSION 4.0)
project(lw3)

set(CMAKE_CXX_STANDARD 20)

add_executable(lw3 main.cpp)

# Подключение библиотеки winmm для функции timeGetTime
target_link_libraries(lw3 winmm)
