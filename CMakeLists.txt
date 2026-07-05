cmake_minimum_required(VERSION 3.10)
project(rm_x86_64_forge)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Enable all warnings
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")

add_executable(rm_x86_64_forge 
    src/main.cc 
    src/tac_parser/three_address_code_parser.cc 
    src/tac_parser/instruction.cc
    src/tac_to_2ac/converter.cc
    src/tac_to_2ac/two_address_instruction.cc
    src/liveness_analysis/cfg.cc
    src/liveness_analysis/basic_block.cc
)
