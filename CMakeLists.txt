cmake_minimum_required(VERSION 3.20)

project(saturn_renderer C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Find SDL2
find_package(SDL2 REQUIRED)

# Create executable
add_executable(saturn_renderer main.c)

# Link libraries
target_link_libraries(saturn_renderer SDL2::SDL2 SDL2::SDL2main)

# Set math library for non-Windows platforms
if(NOT WIN32)
    target_link_libraries(saturn_renderer m)
endif()

# Set compiler flags
target_compile_options(saturn_renderer PRIVATE
    $<$<C_COMPILER_ID:Clang>:-Wall -Wextra -O2>
    $<$<C_COMPILER_ID:GNU>:-Wall -Wextra -O2>
    $<$<C_COMPILER_ID:MSVC>:/W3 /O2>
)

# MSVC specific settings
if(MSVC)
    target_compile_definitions(saturn_renderer PRIVATE _USE_MATH_DEFINES)
endif() 