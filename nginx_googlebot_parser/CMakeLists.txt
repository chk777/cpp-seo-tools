cmake_minimum_required(VERSION 3.10)
project(nginx_googlebot_parser CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(nginx_googlebot_parser parser.cpp)
target_compile_options(nginx_googlebot_parser PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /O2>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -O3>
)
