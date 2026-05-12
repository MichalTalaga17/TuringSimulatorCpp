cmake_minimum_required(VERSION 3.20)
project(TuringSimulatorCpp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(turing_core
    src/Tape.cpp
    src/TuringMachine.cpp
)

target_include_directories(turing_core PUBLIC include)

add_executable(TuringSimulatorCpp main.cpp)
target_link_libraries(TuringSimulatorCpp PRIVATE turing_core)
