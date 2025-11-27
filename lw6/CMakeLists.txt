cmake_minimum_required(VERSION 3.10)
project(lw6_openmp)

set(CMAKE_CXX_STANDARD 17)

# Находим OpenMP
find_package(OpenMP REQUIRED)
if (OPENMP_FOUND)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()

# Добавляем исполняемый файл для задания 1
add_executable(pi_calculator pi_calculator.cpp)

# Подключаем OpenMP к целям
if (OPENMP_FOUND)
    target_link_libraries(pi_calculator OpenMP::OpenMP_CXX)
endif()
