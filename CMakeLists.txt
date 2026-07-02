cmake_minimum_required(VERSION 3.10)
project(rm_x86_64_forge)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Enable all warnings
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")

add_executable(rm_x86_64_forge src/main.cpp)
